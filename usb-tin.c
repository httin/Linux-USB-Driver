#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/err.h> // err()
#include <linux/uaccess.h>

#include "usb-tin.h"

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("USB Registration Driver");

static int dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t dev_read(struct file *f, char __user *buffer, 
                        size_t cnt, loff_t *ppos)
{
    struct my_usb *usb_dev;
    int retval;
    int read_cnt;

    usb_dev = f->private_data;
    printk(KERN_INFO "private data %p\n", f->private_data);
    /* Read the data from the bulk endpoint */
    retval = usb_bulk_msg(usb_dev->device, usb_rcvbulkpipe(usb_dev->device, BULK_EP_IN),
            bulk_buf, MAX_PKT_SIZE, &read_cnt, 5000);

    if (retval)
    {
        printk(KERN_ERR "Bulk message returned %d\n", retval);
        return retval;
    }

    if (copy_to_user(buffer, bulk_buf, (cnt < read_cnt) ? cnt : read_cnt))
        return -EFAULT;

    return (cnt < read_cnt) ? cnt : read_cnt;
}

static ssize_t dev_write(struct file *f, const char __user *buffer, size_t cnt,
                        loff_t *ppos)
{
    struct my_usb *usb_dev;
    int retval;
    int wrote_cnt = (cnt < MAX_PKT_SIZE) ? cnt : MAX_PKT_SIZE;

    usb_dev = f->private_data;

    if (copy_from_user(bulk_buf, buffer, (cnt < MAX_PKT_SIZE) ? cnt : MAX_PKT_SIZE))
        return -EFAULT;

    /* Write the data into the bulk endpoint */
    retval = usb_bulk_msg(usb_dev->device, usb_sndbulkpipe(usb_dev->device, BULK_EP_OUT),
            bulk_buf, (cnt < MAX_PKT_SIZE) ? cnt : MAX_PKT_SIZE, &wrote_cnt, 5000);

    if (retval)
    {
        printk(KERN_ERR "Bulk message returned %d\n", retval);
        return retval;
    }

    return wrote_cnt;
}

static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

static struct usb_class_driver class = {
    .name = "usb/%d",
    .fops = &my_fops,
    .minor_base = 48,
};

static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct my_usb *usb_dev;
    struct usb_endpoint_descriptor *bulk_in, *bulk_out;
    int retval;

    usb_info(interface, id);
    
    /* allocate memory for our private device data */
    usb_dev = kzalloc(sizeof(*usb_dev), GFP_KERNEL);
    if(!usb_dev)
        return -ENOMEM;

    usb_dev->device = usb_get_dev(interface_to_usbdev(interface));
    usb_dev->interface = interface;

    /* set up the endpoint information */
    /* use only the first bulk-in and bulk-out endpoints */
    retval = usb_find_common_endpoints(interface->cur_altsetting,
            &bulk_in, &bulk_out, NULL, NULL);
    if (retval) {
        dev_err(&interface->dev,
            "Could not find both bulk-in and bulk-out endpoints\n");
        goto error;
    }

    /* save our data pointer in this interface device */
    usb_set_intfdata(interface, usb_dev);

    /* we can register the device now, as it is ready */
    retval = usb_register_dev(interface, &class);
    if (retval) {
        /* something prevented us from registering this driver */
        dev_err(&interface->dev,
            "Not able to get a minor for this device.\n");
        usb_set_intfdata(interface, NULL);
        goto error;
    }

    /* let the user know what node this device is now attached to */
    dev_info(&interface->dev,
         "USB device now attached to USBSkel-%d, private data %p",
         interface->minor, usb_dev);
    return 0;

error:
    kfree(usb_dev);
    return retval;
}

static void skel_disconnect(struct usb_interface *interface)
{
    struct my_usb *usb_dev;
    int minor = interface->minor;

    usb_dev = usb_get_intfdata(interface);
    usb_set_intfdata(interface, NULL);

    printk(KERN_INFO "If# %d: now disconnected\n",
        interface->cur_altsetting->desc.bInterfaceNumber);

    usb_deregister_dev(interface, &class);

    /* deallocate memory for private data */
    kfree(usb_dev);

    dev_info(&interface->dev, "USB #%d now disconnected", minor);
}

/* table of devices that work with this driver */
static struct usb_device_id usb_id_table[] =
{
    { USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) },
    { } /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, usb_id_table);

/*
 * Function pointers are called when a device that matches the information
 * provided in the id_table variable is either seen or removed 
 */
static struct usb_driver my_usb_driver =
{
    .name = "my_usb_driver",
    .probe = skel_probe,
    .disconnect = skel_disconnect,
    .id_table = usb_id_table,
};


static int __init usb_init(void) 
{
    int result;
    if ( (result = usb_register(&my_usb_driver)) < 0 ) {
        printk(KERN_WARNING "usb_register failed for the %s. "
            "Error number %d", my_usb_driver.name, result);
        return -1;
    }
    return 0;
}
module_init(usb_init);
static void __exit usb_exit(void) 
{
  usb_deregister(&my_usb_driver);
}
module_exit(usb_exit);
