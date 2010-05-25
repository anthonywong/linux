/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/* Implements an interface between KGSL and the DRM subsystem.  For now this
 * is pretty simple, but it will take on more of the workload as time goes
 * on
 */
#include "drmP.h"
#include "drm.h"
#include <linux/android_pmem.h>

#include "kgsl_drm.h"

#define DRIVER_AUTHOR           "Qualcomm"
#define DRIVER_NAME             "kgsl"
#define DRIVER_DESC             "KGSL DRM"
#define DRIVER_DATE             "20090810"

#define DRIVER_MAJOR            2
#define DRIVER_MINOR            0
#define DRIVER_PATCHLEVEL       0

struct drm_kgsl_gem_object {
	struct drm_gem_object *obj;
	uint32_t phys;
	void *kmem;
	uint32_t type;
	uint64_t mmap_offset;
	int flags;
};

/* TODO:
 * Add vsync wait */

static int kgsl_library_name(struct drm_device *dev, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "yamato");
}

static int kgsl_drm_load(struct drm_device *dev, unsigned long flags)
{
	return 0;
}

static int kgsl_drm_unload(struct drm_device *dev)
{
	return 0;
}

void kgsl_drm_lastclose(struct drm_device *dev)
{
}

void kgsl_drm_preclose(struct drm_device *dev, struct drm_file *file_priv)
{
}

static int kgsl_drm_suspend(struct drm_device *dev, pm_message_t state)
{
	return 0;
}

static int kgsl_drm_resume(struct drm_device *dev)
{
	return 0;
}

static void
kgsl_gem_free_mmap_offset(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;
	struct drm_gem_mm *mm = dev->mm_private;
	struct drm_kgsl_gem_object *priv = obj->driver_private;
	struct drm_map_list *list;

	list = &obj->map_list;
	drm_ht_remove_item(&mm->offset_hash, &list->hash);
	if (list->file_offset_node) {
		drm_mm_put_block(list->file_offset_node);
		list->file_offset_node = NULL;
	}

	kfree(list->map);
	list->map = NULL;

	priv->mmap_offset = 0;
}

static int
kgsl_gem_memory_allocated(struct drm_gem_object *obj)
{
	struct drm_kgsl_gem_object *priv = obj->driver_private;

	switch (priv->type) {
	case DRM_KGSL_GEM_TYPE_EBI:
	case DRM_KGSL_GEM_TYPE_SMI:
		return priv->phys ? 1 : 0;
	case DRM_KGSL_GEM_TYPE_KMEM:
	case DRM_KGSL_GEM_TYPE_KMEM_NOCACHE:
		return priv->kmem ? 1 : 0;
	}

	return 0;
}

static int
kgsl_gem_alloc_memory(struct drm_gem_object *obj)
{
	struct drm_kgsl_gem_object *priv = obj->driver_private;

	/* Return if the memory is already allocated */

	if (kgsl_gem_memory_allocated(obj))
		return 0;

	switch (priv->type) {
	case DRM_KGSL_GEM_TYPE_EBI:
	case DRM_KGSL_GEM_TYPE_SMI:
	{
		int type = priv->type == DRM_KGSL_GEM_TYPE_EBI ?
			PMEM_MEMTYPE_EBI1 : PMEM_MEMTYPE_SMI;

		priv->phys = pmem_kalloc(obj->size,
					 type | PMEM_ALIGNMENT_4K);

		if (IS_ERR((void *) priv->phys)) {
			DRM_ERROR("Error allocating PMEM memory\n");
			priv->phys = 0;
			return -ENOMEM;
		}
	}
	break;
	case DRM_KGSL_GEM_TYPE_KMEM:
	case DRM_KGSL_GEM_TYPE_KMEM_NOCACHE:
	{
		priv->kmem = (void *) vmalloc_user(obj->size);

		if (priv->kmem == NULL)
			return -ENOMEM;
	}
	break;
	}

	return 0;
}

static void
kgsl_gem_free_memory(struct drm_gem_object *obj)
{
	struct drm_kgsl_gem_object *priv = obj->driver_private;

	if (!kgsl_gem_memory_allocated(obj))
		return;

	switch (priv->type) {
	case DRM_KGSL_GEM_TYPE_EBI:
	case DRM_KGSL_GEM_TYPE_SMI:
		pmem_kfree(priv->phys);
		break;

	case DRM_KGSL_GEM_TYPE_KMEM:
	case DRM_KGSL_GEM_TYPE_KMEM_NOCACHE:
		vfree(priv->kmem);
		break;
	}

	priv->phys = 0;
	priv->kmem = NULL;
}


int
kgsl_gem_init_object(struct drm_gem_object *obj)
{
	struct drm_kgsl_gem_object *priv;
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	obj->driver_private = priv;
	priv->obj = obj;

	return 0;
}

void
kgsl_gem_free_object(struct drm_gem_object *obj)
{
	kgsl_gem_free_memory(obj);
	kgsl_gem_free_mmap_offset(obj);
	kfree(obj->driver_private);
}

static int
kgsl_gem_create_mmap_offset(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;
	struct drm_gem_mm *mm = dev->mm_private;
	struct drm_kgsl_gem_object *priv = obj->driver_private;
	struct drm_map_list *list;

	list = &obj->map_list;
	list->map = kzalloc(sizeof(struct drm_map_list), GFP_KERNEL);
	if (list->map == NULL)
		return -ENOMEM;

	list->map->type = _DRM_GEM;
	list->map->size = obj->size;
	list->map->handle = obj;

	/* Allocate a mmap offset */
	list->file_offset_node = drm_mm_search_free(&mm->offset_manager,
						    obj->size / PAGE_SIZE,
						    0, 0);

	if (!list->file_offset_node) {
		DRM_ERROR("Failed to allocate offset for %d\n", obj->name);
		kfree(list->map);
		return -ENOMEM;
	}

	list->file_offset_node = drm_mm_get_block(list->file_offset_node,
						  obj->size / PAGE_SIZE, 0);

	if (!list->file_offset_node) {
		kfree(list->map);
		return -ENOMEM;
	}

	list->hash.key = list->file_offset_node->start;
	if (drm_ht_insert_item(&mm->offset_hash, &list->hash)) {
		DRM_ERROR("Failed to add to map hash\n");
		drm_mm_put_block(list->file_offset_node);
		kfree(list->map);
		return -ENOMEM;
	}

	priv->mmap_offset = ((uint64_t) list->hash.key) << PAGE_SHIFT;

	return 0;
}

int
kgsl_gem_obj_addr(int drm_fd, int handle, unsigned long *start,
			unsigned long *len)
{
	struct file *filp;
	struct drm_device *dev;
	struct drm_file *file_priv;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;

	filp = fget(drm_fd);
	if (unlikely(filp == NULL)) {
		printk(KERN_ERR "%s:%u\n", __func__, __LINE__);
		return -EINVAL;
	}
	file_priv = filp->private_data;
	if (unlikely(file_priv == NULL)) {
		printk(KERN_ERR "%s:%u\n", __func__, __LINE__);
		fput(filp);
		return -EINVAL;
	}
	dev = file_priv->minor->dev;
	if (unlikely(dev == NULL)) {
		printk(KERN_ERR "%s:%u\n", __func__, __LINE__);
		fput(filp);
		return -EINVAL;
	}

	obj = drm_gem_object_lookup(dev, file_priv, handle);
	if (unlikely(obj == NULL)) {
		printk(KERN_ERR "%s:%u\n", __func__, __LINE__);
		fput(filp);
		return -EINVAL;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	*start = priv->pmem_phys;
	/* priv->mmap_offset is used for virt addr */
	*len = obj->size;
	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	fput(filp);
	return 0;
}

int
kgsl_gem_create_ioctl(struct drm_device *dev, void *data,
		      struct drm_file *file_priv)
{
	struct drm_kgsl_gem_create *create = data;
	struct drm_gem_object *obj;
	int ret, handle;
	struct drm_kgsl_gem_object *priv;

	obj = drm_gem_object_alloc(dev, create->size);

	if (obj == NULL)
		return -ENOMEM;

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	priv->phys = 0;
	priv->kmem = NULL;

	/* To preserve backwards compatability, the default memory source
	   is EBI */

	priv->type = DRM_KGSL_GEM_TYPE_EBI;

	ret = drm_gem_handle_create(file_priv, obj, &handle);

	drm_gem_object_handle_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	if (ret)
		return ret;

	create->handle = handle;
	return 0;
}

int
kgsl_gem_setmemtype_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_kgsl_gem_memtype *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret = 0;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL)
		return -EINVAL;

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	switch (args->type) {
	case DRM_KGSL_GEM_TYPE_EBI:
	case DRM_KGSL_GEM_TYPE_KMEM:
	case DRM_KGSL_GEM_TYPE_KMEM_NOCACHE:
#ifdef CONFIG_PMEM_SMI_REGION
	case DRM_KGSL_GEM_TYPE_SMI:
#endif
		priv->type = args->type;
		break;

	default:
		ret = -EINVAL;
	}

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int
kgsl_gem_getmemtype_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	struct drm_kgsl_gem_memtype *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL)
		return -EINVAL;

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	args->type = priv->type;

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

/* Allocate the memory and prepare it for CPU mapping */

int
kgsl_gem_alloc_ioctl(struct drm_device *dev, void *data,
		    struct drm_file *file_priv)
{
	struct drm_kgsl_gem_alloc *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL)
		return -EINVAL;

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	ret = kgsl_gem_alloc_memory(obj);

	if (ret) {
		DRM_ERROR("Unable to allocate memory\n");
	} else if (!priv->mmap_offset) {
		ret = kgsl_gem_create_mmap_offset(obj);
		if (ret)
			DRM_ERROR("Unable to create a mmap offset\n");
	}

	args->offset = priv->mmap_offset;

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int
kgsl_gem_mmap_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_kgsl_gem_mmap *args = data;
	struct drm_gem_object *obj;
	unsigned long addr;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL)
		return -EINVAL;

	down_write(&current->mm->mmap_sem);

	addr = do_mmap(obj->filp, 0, args->size,
		       PROT_READ | PROT_WRITE, MAP_SHARED,
		       args->offset);

	up_write(&current->mm->mmap_sem);

	mutex_lock(&dev->struct_mutex);
	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	if (IS_ERR((void *) addr))
		return addr;

	args->hostptr = (uint32_t) addr;
	return 0;
}

/* This function is deprecated */

int
kgsl_gem_prep_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_kgsl_gem_prep *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL)
		return -EINVAL;

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	ret = kgsl_gem_alloc_memory(obj);
	if (ret) {
		DRM_ERROR("Unable to allocate memory\n");
		drm_gem_object_unreference(obj);
		mutex_unlock(&dev->struct_mutex);
		return ret;
	}

	if (priv->mmap_offset == 0) {
		ret = kgsl_gem_create_mmap_offset(obj);
		if (ret) {
			drm_gem_object_unreference(obj);
			mutex_unlock(&dev->struct_mutex);
			return ret;
		}
	}

	args->offset = priv->mmap_offset;
	args->phys = priv->phys;

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

int kgsl_gem_kmem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	struct drm_device *dev = obj->dev;
	struct drm_kgsl_gem_object *priv;
	unsigned long offset, pg;
	struct page *page;

	mutex_lock(&dev->struct_mutex);

	priv = obj->driver_private;

	offset = (unsigned long) vmf->virtual_address - vma->vm_start;
	pg = (unsigned long) priv->kmem + offset;

	page = vmalloc_to_page((void *) pg);
	if (!page) {
		mutex_unlock(&dev->struct_mutex);
		return VM_FAULT_SIGBUS;
	}

	get_page(page);
	vmf->page = page;

	mutex_unlock(&dev->struct_mutex);
	return 0;
}

int kgsl_gem_phys_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	struct drm_device *dev = obj->dev;
	struct drm_kgsl_gem_object *priv;
	unsigned long offset, pfn;
	int ret = 0;

	offset = ((unsigned long) vmf->virtual_address - vma->vm_start) >>
		PAGE_SHIFT;

	mutex_lock(&dev->struct_mutex);

	priv = obj->driver_private;

	pfn = (priv->phys >> PAGE_SHIFT) + offset;
	ret = vm_insert_pfn(vma,
			    (unsigned long) vmf->virtual_address, pfn);
	mutex_unlock(&dev->struct_mutex);

	switch (ret) {
	case -ENOMEM:
	case -EAGAIN:
		return VM_FAULT_OOM;
	case -EFAULT:
		return VM_FAULT_SIGBUS;
	default:
		return VM_FAULT_NOPAGE;
	}
}

static struct vm_operations_struct kgsl_gem_kmem_vm_ops = {
	.fault = kgsl_gem_kmem_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static struct vm_operations_struct kgsl_gem_phys_vm_ops = {
	.fault = kgsl_gem_phys_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

/* This is a clone of the standard drm_gem_mmap function modified to allow
   us to properly map KMEM regions as well as the PMEM regions */

int msm_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_file *priv = filp->private_data;
	struct drm_device *dev = priv->minor->dev;
	struct drm_gem_mm *mm = dev->mm_private;
	struct drm_map *map = NULL;
	struct drm_gem_object *obj;
	struct drm_hash_item *hash;
	struct drm_kgsl_gem_object *gpriv;
	int ret = 0;

	mutex_lock(&dev->struct_mutex);

	if (drm_ht_find_item(&mm->offset_hash, vma->vm_pgoff, &hash)) {
		mutex_unlock(&dev->struct_mutex);
		return drm_mmap(filp, vma);
	}

	map = drm_hash_entry(hash, struct drm_map_list, hash)->map;
	if (!map ||
	    ((map->flags & _DRM_RESTRICTED) && !capable(CAP_SYS_ADMIN))) {
		ret =  -EPERM;
		goto out_unlock;
	}

	/* Check for valid size. */
	if (map->size < vma->vm_end - vma->vm_start) {
		ret = -EINVAL;
		goto out_unlock;
	}

	obj = map->handle;

	gpriv = obj->driver_private;

	/* VM_PFNMAP is only for memory that doesn't use struct page
	 * in other words, not "normal" memory.  If you try to use it
	 * with "normal" memory then the mappings don't get flushed. */

	if (gpriv->type == DRM_KGSL_GEM_TYPE_KMEM ||
		gpriv->type == DRM_KGSL_GEM_TYPE_KMEM_NOCACHE) {
		vma->vm_flags |= VM_RESERVED | VM_DONTEXPAND;
		vma->vm_ops = &kgsl_gem_kmem_vm_ops;
	} else {
		vma->vm_flags |= VM_RESERVED | VM_IO | VM_PFNMAP |
			VM_DONTEXPAND;
		vma->vm_ops = &kgsl_gem_phys_vm_ops;
	}

	vma->vm_private_data = map->handle;

	switch (gpriv->type) {
	case DRM_KGSL_GEM_TYPE_KMEM_NOCACHE:
	case DRM_KGSL_GEM_TYPE_EBI:
	case DRM_KGSL_GEM_TYPE_SMI:
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		break;
	case DRM_KGSL_GEM_TYPE_KMEM:
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	}

	/* Take a ref for this mapping of the object, so that the fault
	 * handler can dereference the mmap offset's pointer to the object.
	 * This reference is cleaned up by the corresponding vm_close
	 * (which should happen whether the vma was created by this call, or
	 * by a vm_open due to mremap or partial unmap or whatever).
	 */
	drm_gem_object_reference(obj);

	vma->vm_file = filp;	/* Needed for drm_vm_open() */
	drm_vm_open_locked(vma);

out_unlock:
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

struct drm_ioctl_desc kgsl_drm_ioctls[] = {
	DRM_IOCTL_DEF(DRM_KGSL_GEM_CREATE, kgsl_gem_create_ioctl, 0),
	DRM_IOCTL_DEF(DRM_KGSL_GEM_PREP, kgsl_gem_prep_ioctl, 0),
	DRM_IOCTL_DEF(DRM_KGSL_GEM_SETMEMTYPE, kgsl_gem_setmemtype_ioctl, 0),
	DRM_IOCTL_DEF(DRM_KGSL_GEM_GETMEMTYPE, kgsl_gem_getmemtype_ioctl, 0),
	DRM_IOCTL_DEF(DRM_KGSL_GEM_ALLOC, kgsl_gem_alloc_ioctl, 0),
	DRM_IOCTL_DEF(DRM_KGSL_GEM_MMAP, kgsl_gem_mmap_ioctl, 0),
};

static struct drm_driver driver = {
	.driver_features = DRIVER_USE_PLATFORM_DEVICE | DRIVER_GEM,
	.load = kgsl_drm_load,
	.unload = kgsl_drm_unload,
	.lastclose = kgsl_drm_lastclose,
	.preclose = kgsl_drm_preclose,
	.suspend = kgsl_drm_suspend,
	.resume = kgsl_drm_resume,
	.reclaim_buffers = drm_core_reclaim_buffers,
	.get_map_ofs = drm_core_get_map_ofs,
	.get_reg_ofs = drm_core_get_reg_ofs,
	.gem_init_object = kgsl_gem_init_object,
	.gem_free_object = kgsl_gem_free_object,
	.ioctls = kgsl_drm_ioctls,

	.fops = {
		 .owner = THIS_MODULE,
		 .open = drm_open,
		 .release = drm_release,
		 .ioctl = drm_ioctl,
		 .mmap = msm_drm_gem_mmap,
		 .poll = drm_poll,
		 .fasync = drm_fasync,
		 },

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

int kgsl_drm_init(struct platform_device *dev)
{
	driver.num_ioctls = DRM_ARRAY_SIZE(kgsl_drm_ioctls);
	driver.platform_device = dev;
	return drm_init(&driver);
}

void kgsl_drm_exit(void)
{
	drm_exit(&driver);
}
