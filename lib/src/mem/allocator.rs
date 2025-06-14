use std::{ffi::c_void, mem::MaybeUninit, num::NonZero, ptr::{slice_from_raw_parts_mut, NonNull}};

#[repr(C)]
pub struct Allocator {
    ptr: *mut c_void,
    vtable: *const AllocatorVTable
}

#[repr(C)]
pub struct AllocatorVTable {
    pub alloc_fn: fn(ptr: *mut c_void, len: usize, align: usize) -> *mut c_void,
    pub free_fn: fn(ptr: *mut c_void, buf: *mut c_void, len: usize, align: usize)
}

pub type AllocError = ();

impl Default for Allocator {
    fn default() -> Self {
        unsafe {
            let mut alloc = MaybeUninit::<Allocator>::zeroed();
            let ptr = alloc.as_mut_ptr();
            let allocator_ptr = &mut (*ptr) as *mut Allocator;
            std::ptr::copy(sy_default_allocator(), allocator_ptr, 1);
            return alloc.assume_init();
        }
    }
}

impl Allocator {
    pub fn init(allocator: &mut Box<dyn IAllocator>) -> Allocator {
        let boxed_allocator = Box::new(allocator);
        let as_ptr: *mut c_void = unsafe { std::mem::transmute(boxed_allocator) }; // same size
        return Self{ptr: as_ptr, vtable: IALLOCATOR_BOX_VTABLE};
    }

    pub fn alloc_object<T>(self: &mut Self) -> Result<NonNull<T>, AllocError> {
        return self.alloc_aligned_object(NonZero::new(align_of::<T>()).expect("Cannot allocate memory for zero aligned type"));
    }

    pub fn alloc_array<T>(self: &mut Self, len: NonZero<usize>) -> Result<NonNull<[T]>, AllocError> {
        return self.alloc_aligned_array(len, NonZero::new(align_of::<T>()).expect("Cannot allocate memory for zero aligned type"));
    }

    pub fn alloc_aligned_object<T>(self: &mut Self, align: NonZero<usize>) -> Result<NonNull<T>, AllocError> {
        let actual_align = if align_of::<T>() > align.get() { align_of::<T>() } else { align.get() };
        let allocation = unsafe {sy_allocator_alloc(self as *mut Self, size_of::<T>(), actual_align)};
        if allocation == std::ptr::null_mut() {
            return Err(());
        }
        return Ok(unsafe {NonNull::new_unchecked(allocation as *mut T)});
    }

    pub fn alloc_aligned_array<T>(self: &mut Self, len: NonZero<usize>, align: NonZero<usize>) -> Result<NonNull<[T]>, AllocError> {
        let actual_align = if align_of::<T>() > align.get() { align_of::<T>() } else { align.get() };
        let allocation = unsafe {sy_allocator_alloc(self as *mut Self, size_of::<T>() * len.get(), actual_align)};
        if allocation == std::ptr::null_mut() {
            return Err(());
        }
        let slice = slice_from_raw_parts_mut(allocation as *mut T, len.get());
        return Ok(unsafe {NonNull::new_unchecked(slice)});
    }

    pub fn free_object<T>(self: &mut Self, obj: NonNull<T>) {
        self.free_aligned_object(obj, NonZero::new(align_of::<T>()).expect("Cannot free memory for zero aligned type"));
    }

    pub fn free_array<T>(self: &mut Self, arr: NonNull<[T]>) {
        self.free_aligned_array(arr, NonZero::new(align_of::<T>()).expect("Cannot free memory for zero aligned type"));
    }

    pub fn free_aligned_object<T>(self: &mut Self, obj: NonNull<T>, align: NonZero<usize>) {
        let actual_align = if align_of::<T>() > align.get() { align_of::<T>() } else { align.get() };
        let ptr = obj.as_ptr() as *mut c_void;
        unsafe { sy_allocator_free(self as *mut Self, ptr, size_of::<T>(), actual_align) };
    }

    pub fn free_aligned_array<T>(self: &mut Self, arr: NonNull<[T]>, align: NonZero<usize>) {
        let actual_align = if align_of::<T>() > align.get() { align_of::<T>() } else { align.get() };
        let ptr = arr.as_ptr() as *mut c_void;
        let len = arr.len();
        unsafe { sy_allocator_free(self as *mut Self, ptr, size_of::<T>() * len, actual_align) };
    }
}

pub trait IAllocator {
    fn alloc(self: &mut Self, len: NonZero<usize>, align: NonZero<usize>) -> *mut c_void;
    fn free(self: &mut Self, buf: NonNull<c_void>, len: NonZero<usize>, align: NonZero<usize>);
}

fn iallocator_alloc_box(opaque: *mut c_void, len: usize, align: usize) -> *mut c_void {
    let mut allocator: Box<Box<dyn IAllocator>> = unsafe {std::mem::transmute(opaque)}; // same size and align
    let len = NonZero::new(len).expect("Cannot allocate memory of zero length");
    let align = NonZero::new(align).expect("Cannot allocate memory of zero alignment");
    return (*allocator).alloc(len, align);
}

fn iallocator_free_box(opaque: *mut c_void, buf: *mut c_void, len: usize, align: usize) {
    let mut allocator: Box<Box<dyn IAllocator>> = unsafe {std::mem::transmute(opaque)}; // same size and align
    let len = NonZero::new(len).expect("Cannot allocate memory of zero length");
    let align = NonZero::new(align).expect("Cannot allocate memory of zero alignment");
    let buf = NonNull::new(buf).expect("Cannot free null pointer");
    (*allocator).free(buf, len, align);
}

const IALLOCATOR_BOX_VTABLE: *const AllocatorVTable = &AllocatorVTable{
    alloc_fn: iallocator_alloc_box,
    free_fn: iallocator_free_box,
};

unsafe extern "C" {  
    pub fn sy_allocator_alloc(allocator: *mut Allocator, len: usize, align: usize) -> *mut c_void; 
    pub fn sy_allocator_free(allocator: *mut Allocator, buf: *mut c_void, len: usize, align: usize);
}

unsafe extern "C" {
    unsafe static sy_defaultAllocator: *mut c_void;
}

pub fn sy_default_allocator() -> *mut Allocator {
    return unsafe { sy_defaultAllocator as *mut Allocator };
}


#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn size_of_vtable() {
        assert_eq!(size_of::<AllocatorVTable>(), 16);
    }
}
