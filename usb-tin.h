#ifndef _USB_TIN_H_
#define _USB_TIN_H_

#define USB_VENDOR_ID 0x1687
#define USB_PRODUCT_ID 0x3257
#define MAX_PKT_SIZE 512

/* private data for specific USB device */
struct my_usb {
    struct usb_device *device; /* kernel's representation of a USB device */
    struct usb_interface *interface; /* what usb device drivers talk to */
	struct urb		*bulk_in_urb;		/* the urb to read data with */

	unsigned char	*bulk_in_buffer;	/* the buffer to receive data */
	size_t			bulk_in_size;		/* the size of the receive buffer */
	size_t			bulk_in_filled;		/* number of bytes in the buffer */
	size_t			bulk_in_copied;		/* already copied to user space */
	struct kref kref;	/* generic reference counted objects */

	__u8 			bulk_in_endpointAddr;
	__u8			bulk_out_endpointAddr;
};

unsigned char bulk_buf[MAX_PKT_SIZE];

/* file_operations function that enable user to interact with USB file device */
static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
static ssize_t dev_read(struct file *f, char __user *buffer, 
                        size_t count, loff_t *ppos);
static ssize_t dev_write(struct file *f, const char __user *buffer, 
						size_t count, loff_t *ppos);

static void usb_info (struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    int i;

    iface_desc = interface->cur_altsetting;

    printk(KERN_INFO "If# %d: (%04X:%04X) now probed\n",
            iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);
    printk(KERN_INFO "ID->bNumEndpoints: 0x%02X\n",
            iface_desc->desc.bNumEndpoints);
    printk(KERN_INFO "ID->bInterfaceClass: 0x%02X\n",
            iface_desc->desc.bInterfaceClass);

 
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++)
    {
        endpoint = &iface_desc->endpoint[i].desc;

        printk(KERN_INFO "ED[%d]->bEndpointAddress: 0x%02X\n",
                i, endpoint->bEndpointAddress);
        printk(KERN_INFO "ED[%d]->bmAttributes: 0x%02X\n",
                i, endpoint->bmAttributes);
        printk(KERN_INFO "ED[%d]->wMaxPacketSize: 0x%04X (%d)\n",
                i, endpoint->wMaxPacketSize, endpoint->wMaxPacketSize);
    }
}

#endif