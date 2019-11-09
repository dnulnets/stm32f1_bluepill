/*
** USB CDC (ACM) - USB Communication Device Class (Abstract Control Model)
**
** Modified by Tomas Stenlund, Stenlund Open Source Group, 2019
**
** Original code taken from libopencm3 example for STM32 F1 examples. Modified to
** be interrupot driven instead and adds simple printing functions so we do not need
** to use the entire C std library on a system with smal flash.
**
*/

/* Standard C library */
#include <stdlib.h>

/* libopencm3 library */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

/* My own headers */
#include "delay.h"
#include "usb.h"

/* USB Transfer buffer size */
#define USB_XFER_BUFFER_SIZE 64
/* USB transmit timeout value, 50 ticks */
#define USB_TX_TIMEOUT_TICKS 50

/* The USB device */
static usbd_device *my_usbd_dev = NULL;

/* USB Control requests */
#define USB_CTRL_BUFFER_SIZE 128
static uint8_t usbd_control_buffer[USB_CTRL_BUFFER_SIZE];

/* Status flag that tells us that we are connected, set by RTS hardware control */
static bool usb_connected = false;

/* USB Receive ring buffer */
#define USB_RING_BUFFER_LENGTH 128
static uint8_t usbd_receive_buffer[USB_RING_BUFFER_LENGTH];
static volatile int rix_in = 0, rix_out = 0;

/* Line coding, not defined by libopemcm3, we will use 1152008N1 */
#define USB_CDC_REQ_GET_LINE_CODING 0x21
static const struct usb_cdc_line_coding line_coding = {
	.dwDTERate = 115200,
	.bCharFormat = USB_CDC_1_STOP_BITS,
	.bParityType = USB_CDC_NO_PARITY,
	.bDataBits = 0x08
};

/* The USB device definition structures */
static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/* The control endpoint */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

/* The read and write endpoint */
static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 },
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Stenlund Open Source Group",
	"STM32 Simple Framework",
	"1",
};

/*
** Enable the USB interrupts
*/
static void usb_ints_enable(void)
{
	nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);
	nvic_enable_irq(NVIC_USB_HP_CAN_TX_IRQ);
	nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
}

/*
** Disable the USB interrupts
*/
static void usb_ints_disable(void)
{
	nvic_disable_irq(NVIC_USB_WAKEUP_IRQ);
	nvic_disable_irq(NVIC_USB_HP_CAN_TX_IRQ);
	nvic_disable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
}

/*
** USB Control requests callback
*/
static enum usbd_request_return_codes usb_control_request(usbd_device *usbd_dev,
	struct usb_setup_data *req, uint8_t **buf,
	uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;
	(void)usbd_dev;

	/* Decide what to do with the control request */
	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
		uint16_t rtsdtr = req->wValue;	// DTR is bit 0, RTS is bit 1
		usb_connected = ((rtsdtr & 1) != 0) && ((rtsdtr & 2)!=0);
		return USBD_REQ_HANDLED;
		}
	case USB_CDC_REQ_SET_LINE_CODING: {
		if (*len < sizeof(struct usb_cdc_line_coding))
			return USBD_REQ_NOTSUPP;
		return USBD_REQ_HANDLED;
		}
	case USB_CDC_REQ_GET_LINE_CODING: {
		*buf = (uint8_t *)&line_coding;
		return USBD_REQ_HANDLED;
		}
	}
	return USBD_REQ_NOTSUPP;
}

/*
** Callback for a character that has arrived to the USB port
*/
static void usb_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	(void)usbd_dev;

	/* We only have endpoint 0x01 receiving data */
	if (ep == 0x01) {

		/* We do not want any more callbacks from the polling while being in a callback */
		usb_ints_disable();

		/* Loop through the buffer and add it to the ring buffer */
		char buf[64];
		int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);
		if (len>0) {

			/* Echo back what is written */
			usbd_ep_write_packet(usbd_dev, 0x82, buf, len);

			/* Store it in the ring buffer */
			for (int i = 0; i<len; i++) {

				/* Transfer the character to the ring buffer and increase the counter */
				usbd_receive_buffer [rix_in] = buf[i];				
				rix_in = (rix_in+1) % USB_RING_BUFFER_LENGTH;

				/* If we have a buffer overrun, then drop the oldest character */
				if (rix_in == rix_out)										
					rix_out = (rix_out+1) % USB_RING_BUFFER_LENGTH;
			}
		}

		/* Enable the polling again */
		usb_ints_enable();
	}
}

/*
** raw reading until CR
*/
void usb_readln (void *data, uint32_t size)

{
	uint32_t i = 0;
	bool end = false;
	register uint8_t ch;

	/* Do not return until we have a complete line, ending with a CR */
	while (!end && i<size) {

		/* Get all the data we have */
		while (rix_out != rix_in) {
			ch = ((uint8_t *)data)[i++] = usbd_receive_buffer[rix_out];
			rix_out = (rix_out + 1) % USB_RING_BUFFER_LENGTH;
			if (ch == 0x0d) {
				end = true;
				i--;
				break;
			}	
		}
	}
	((uint8_t *)data)[i] = 0;
}

/*
** Raw sending data
*/
void usb_send(const void *data, uint32_t len)
{
	/* We do not want any polling to be done while sending */
	usb_ints_disable();

	/* Send until we have sent everything or timed out */
	while (len)
	{
		uint32_t cur = len;

		/* We can only send the max buffer size each USB transmit */
		if (cur > USB_XFER_BUFFER_SIZE)
		{
			cur = USB_XFER_BUFFER_SIZE;
		}

		uint8_t timeout = 0;
		volatile uint32_t usb_tx_start_tick = tickcount();
		while (usbd_ep_write_packet(my_usbd_dev, 0x82, data, cur) == 0)
		{
			/* Immediately timeout, if the last send timed out or if we time out now */
			if (tickcount() - usb_tx_start_tick > USB_TX_TIMEOUT_TICKS )
			{
				timeout = 1;
				break;
			}

			/* Allow for some interrupts polling */
			usb_ints_enable();
			usb_ints_disable();
		}

		/* If we had any, get out of here */
		if (timeout)
		{
			break;
		}

		len -= cur;
		data += cur;
	}

	/* Get the polling running again */
	usb_ints_enable();
}

/*
** TX Interrupt
*/
void usb_hp_can_tx_isr(void)
{
	/* Poll the device */
	usbd_poll(my_usbd_dev);
}

/*
** RX Interrupt
*/
void usb_lp_can_rx0_isr(void)
{
	/* Poll the device */
	usbd_poll(my_usbd_dev);
}

/*
** Wakeup Interrupt
*/
void usb_wakeup_isr(void)
{
	/* Poll the device */
	usbd_poll(my_usbd_dev);
}

/*
** A configuration callback for the USB device
*/
static void usb_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;

	/* Set up the endpoints, 0x01, 0x82 and 0x83, MSB is direction */
	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, usb_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	/* Register the control callback */
	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				usb_control_request);
}

/*
** Trigger a rescan/enumeration of the device
*/
static void usb_trigger_rescan(void)
{
	/* Trigger rescan by forcing usb data pin 12 on port A to low for 200 ms */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
	gpio_clear(GPIOA, GPIO12);
	delay_ticks(200);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO12);
}

/*
** Initialize the USB device.
**
** You need to have set up the system clocks, GPIOA periph clock and the systick counter already!
**
** 	rcc_clock_setup_in_hse_8mhz_out_72mhz();
**	rcc_periph_clock_enable(RCC_GPIOA);
**  delay_init();
**
*/
void usb_init(void)
{
	usbd_device *usbd_dev;

	/* trigger a USB rescan */
	usb_trigger_rescan();

	/* Init the device */
	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	my_usbd_dev = usbd_dev;

	/* Set up the configuration callback */
	usbd_register_set_config_callback(usbd_dev, usb_set_config);

	/* Set the interrupts priority, 0x88 = medium */
	nvic_set_priority(NVIC_USB_WAKEUP_IRQ, 0x88);
	nvic_set_priority(NVIC_USB_HP_CAN_TX_IRQ, 0x88);
	nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, 0x88);

	/* Enable them, will cause polling to start */
	usb_ints_enable();
}

/*
** Waits for connection, busy loop
*/
void usb_wait_for_connection (void)
{
	while (usb_connected == false)
		__asm__("nop");
	delay_ticks (100);
}