use std::{ffi::c_void, mem::MaybeUninit, num::NonZero, ptr::{slice_from_raw_parts_mut, NonNull}};

#[repr(C)]
pub struct Allocator {
    allocator: SyAllocator
}

pub type AllocError = ();

impl Default for Allocator {
    fn default() -> Self {
        unsafe {
            let mut alloc = MaybeUninit::<Allocator>::zeroed();
            let ptr = alloc.as_mut_ptr();
            let allocator_ptr = &mut (*ptr).allocator as *mut SyAllocator;
            std::ptr::copy(sy_default_allocator(), allocator_ptr, 1);
            return alloc.assume_init();
        }
    }
}

impl Allocator {
    pub fn init(allocator: Box<dyn IAllocator>) -> Allocator {
        let boxed_allocator = Box::new(allocator);
        let as_ptr: *mut c_void = unsafe { std::mem::transmute(boxed_allocator) }; // same size
        let c_allocator = SyAllocator{ptr: as_ptr, vtable: IALLOCATOR_BOX_VTABLE};
        return Self{allocator: c_allocator};
    }

    pub fn alloc_object<T>(self: &mut Self) -> Result<NonNull<T>, AllocError> {
        return self.alloc_aligned_object(NonZero::new(align_of::<T>()).expect("Cannot allocate memory for zero aligned type"));
    }

    pub fn alloc_array<T>(self: &mut Self, len: NonZero<usize>) -> Result<NonNull<[T]>, AllocError> {
        return self.alloc_aligned_array(len, NonZero::new(align_of::<T>()).expect("Cannot allocate memory for zero aligned type"));
    }

    pub fn alloc_aligned_object<T>(self: &mut Self, align: NonZero<usize>) -> Result<NonNull<T>, AllocError> {
        let actual_align = if align_of::<T>() > align.get() { align_of::<T>() } else { align.get() };
        let allocation = unsafe {sy_allocator_alloc(self.c_allocator_void_ptr(), size_of::<T>(), actual_align)};
        if allocation == std::ptr::null_mut() {
            return Err(());
        }
        return Ok(unsafe {NonNull::new_unchecked(allocation as *mut T)});
    }

    pub fn alloc_aligned_array<T>(self: &mut Self, len: NonZero<usize>, align: NonZero<usize>) -> Result<NonNull<[T]>, AllocError> {
        let actual_align = if align_of::<T>() > align.get() { align_of::<T>() } else { align.get() };
        let allocation = unsafe {sy_allocator_alloc(self.c_allocator_void_ptr(), size_of::<T>() * len.get(), actual_align)};
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
        unsafe { sy_allocator_free(self.c_allocator_void_ptr(), ptr, size_of::<T>(), actual_align) };
    }

    pub fn free_aligned_array<T>(self: &mut Self, arr: NonNull<[T]>, align: NonZero<usize>) {
        let actual_align = if align_of::<T>() > align.get() { align_of::<T>() } else { align.get() };
        let ptr = arr.as_ptr() as *mut c_void;
        let len = arr.len();
        unsafe { sy_allocator_free(self.c_allocator_void_ptr(), ptr, size_of::<T>() * len, actual_align) };
    }

    pub fn c_allocator<'a>(self: &'a mut Self) -> &'a mut SyAllocator {
        return &mut self.allocator;
    }

    fn c_allocator_void_ptr(self: &mut Self) -> *mut c_void {
        return &mut self.allocator as *mut SyAllocator as *mut c_void;
    }
}

impl Drop for Allocator {
    fn drop(self: &mut Self) {
        unsafe { sy_allocator_destructor(self.c_allocator_void_ptr()); }
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

fn iallocator_drop_box(opaque: *mut c_void) {
    let _: Box<Box<dyn IAllocator>> = unsafe {std::mem::transmute(opaque)}; // same size and align
}

const IALLOCATOR_BOX_VTABLE: *const SyAllocatorVTable = &SyAllocatorVTable{
    alloc_fn: iallocator_alloc_box,
    free_fn: iallocator_free_box,
    drop_fn: iallocator_drop_box, 
};

#[repr(C)]
pub struct SyAllocatorVTable {
    pub alloc_fn: fn(ptr: *mut c_void, len: usize, align: usize) -> *mut c_void,
    pub free_fn: fn(ptr: *mut c_void, buf: *mut c_void, len: usize, align: usize),
    pub drop_fn: fn(ptr: *mut c_void)
}

#[repr(C)]
pub struct SyAllocator {
    pub ptr: *mut c_void,
    pub vtable: *const SyAllocatorVTable
}

unsafe extern "C" {  
    pub fn sy_allocator_alloc(allocator: *mut c_void, len: usize, align: usize) -> *mut c_void; 
    pub fn sy_allocator_free(allocator: *mut c_void, buf: *mut c_void, len: usize, align: usize);
    pub fn sy_allocator_destructor(allocator: *mut c_void);
}

unsafe extern "C" {
    unsafe static sy_defaultAllocator: *mut c_void;
}

pub const fn sy_default_allocator() -> *mut SyAllocator {
    return unsafe { sy_defaultAllocator as *mut SyAllocator };
}


#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn size_of_vtable() {
        assert_eq!(size_of::<SyAllocatorVTable>(), 24);
    }

    #[test]
    fn size_align_of_nested_box() {        
        assert_eq!(size_of::<Box<Box<dyn IAllocator>>>(), size_of::<*mut c_void>());
        assert_eq!(align_of::<Box<Box<dyn IAllocator>>>(), align_of::<*mut c_void>());
    }

    #[test]
    fn size_of_box_dyn() {
        assert_eq!(size_of::<Box<dyn IAllocator>>(), 16);
    }

    #[test]
    fn size_of_option_fn_ptr() {
        assert_eq!(size_of::<Option<fn(ptr: *mut c_void)>>(), 8);
    }
}
