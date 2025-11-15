#![allow(improper_ctypes)]

use core::marker::PhantomData;
use core::{
    ffi::c_void,
    num::NonZero,
    ptr::{slice_from_raw_parts_mut, NonNull},
};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Allocator<'a> {
    ptr: *const c_void,
    vtable: &'static AllocatorVTable,
    _marker: PhantomData<&'a ()>,
}

#[repr(C)]
pub struct AllocatorVTable {
    pub alloc_fn: fn(ptr: *const c_void, len: usize, align: usize) -> *mut c_void,
    pub free_fn: fn(ptr: *const c_void, buf: *mut c_void, len: usize, align: usize),
}

pub type AllocError = ();

impl Default for Allocator<'static> {
    fn default() -> Self {
        let ptr: *const Allocator = unsafe { sy_defaultAllocator as *const Allocator<'static> };
        let temp_ref: &'static Allocator = unsafe { &*ptr };
        return *temp_ref;
    }
}

impl<'a> Allocator<'a> {
    pub unsafe fn init(ptr: *const c_void, vtable: &'static AllocatorVTable) -> Allocator<'a> {
        return Self {
            ptr: ptr,
            vtable: vtable,
            _marker: PhantomData,
        };
    }

    pub fn alloc_object<T>(self: &Self) -> Result<NonNull<T>, AllocError> {
        return self.alloc_aligned_object(
            NonZero::new(align_of::<T>()).expect("Cannot allocate memory for zero aligned type"),
        );
    }

    pub fn alloc_array<T>(self: &Self, len: NonZero<usize>) -> Result<NonNull<[T]>, AllocError> {
        return self.alloc_aligned_array(
            len,
            NonZero::new(align_of::<T>()).expect("Cannot allocate memory for zero aligned type"),
        );
    }

    pub fn alloc_aligned_object<T>(
        self: &Self,
        align: NonZero<usize>,
    ) -> Result<NonNull<T>, AllocError> {
        let actual_align = if align_of::<T>() > align.get() {
            align_of::<T>()
        } else {
            align.get()
        };
        let allocation =
            unsafe { sy_allocator_alloc(self as *const Self, size_of::<T>(), actual_align) };
        if allocation == core::ptr::null_mut() {
            return Err(());
        }
        return Ok(unsafe { NonNull::new_unchecked(allocation as *mut T) });
    }

    pub fn alloc_aligned_array<T>(
        self: &Self,
        len: NonZero<usize>,
        align: NonZero<usize>,
    ) -> Result<NonNull<[T]>, AllocError> {
        let actual_align = if align_of::<T>() > align.get() {
            align_of::<T>()
        } else {
            align.get()
        };
        let allocation = unsafe {
            sy_allocator_alloc(
                self as *const Self,
                size_of::<T>() * len.get(),
                actual_align,
            )
        };
        if allocation == core::ptr::null_mut() {
            return Err(());
        }
        let slice = slice_from_raw_parts_mut(allocation as *mut T, len.get());
        return Ok(unsafe { NonNull::new_unchecked(slice) });
    }

    pub fn free_object<T>(self: &Self, obj: NonNull<T>) {
        self.free_aligned_object(
            obj,
            NonZero::new(align_of::<T>()).expect("Cannot free memory for zero aligned type"),
        );
    }

    pub fn free_array<T>(self: &Self, arr: NonNull<[T]>) {
        self.free_aligned_array(
            arr,
            NonZero::new(align_of::<T>()).expect("Cannot free memory for zero aligned type"),
        );
    }

    pub fn free_aligned_object<T>(self: &Self, obj: NonNull<T>, align: NonZero<usize>) {
        let actual_align = if align_of::<T>() > align.get() {
            align_of::<T>()
        } else {
            align.get()
        };
        let ptr = obj.as_ptr() as *mut c_void;
        unsafe { sy_allocator_free(self as *const Self, ptr, size_of::<T>(), actual_align) };
    }

    pub fn free_aligned_array<T>(self: &Self, arr: NonNull<[T]>, align: NonZero<usize>) {
        let actual_align = if align_of::<T>() > align.get() {
            align_of::<T>()
        } else {
            align.get()
        };
        let ptr = arr.as_ptr() as *mut c_void;
        let len = arr.len();
        unsafe { sy_allocator_free(self as *const Self, ptr, size_of::<T>() * len, actual_align) };
    }
}

pub trait IAllocator {
    fn alloc(self: &Self, len: NonZero<usize>, align: NonZero<usize>) -> *mut c_void;
    fn free(self: &Self, buf: NonNull<c_void>, len: NonZero<usize>, align: NonZero<usize>);
    fn allocator<'a>(self: &'a Self) -> Allocator<'a>
    where
        Self: Sized,
    {
        let vtable: &'static AllocatorVTable = &AllocatorVTable {
            alloc_fn: iallocator_alloc::<Self>,
            free_fn: iallocator_free::<Self>,
        };
        let trait_ptr: *const dyn IAllocator = self as *const dyn IAllocator;
        let as_ptr: *const c_void = trait_ptr as *const c_void;
        return Allocator {
            ptr: as_ptr,
            vtable: vtable,
            _marker: PhantomData,
        };
    }
}

fn iallocator_alloc<T>(opaque: *const c_void, len: usize, align: usize) -> *mut c_void
where
    T: IAllocator,
    T: Sized,
{
    let thin_ptr: *const T = opaque as *const T;
    let trait_ptr: *const dyn IAllocator = thin_ptr;
    let allocator: &dyn IAllocator = unsafe { &*trait_ptr };

    let len = NonZero::new(len).expect("Cannot allocate memory of zero length");
    let align = NonZero::new(align).expect("Cannot allocate memory of zero alignment");
    return allocator.alloc(len, align);
}

fn iallocator_free<T>(opaque: *const c_void, buf: *mut c_void, len: usize, align: usize)
where
    T: IAllocator,
    T: Sized,
{
    let thin_ptr: *mut T = opaque as *mut T;
    let trait_ptr: *mut dyn IAllocator = thin_ptr;
    let allocator: &mut dyn IAllocator = unsafe { &mut *trait_ptr };

    let len = NonZero::new(len).expect("Cannot free memory of zero length");
    let align = NonZero::new(align).expect("Cannot free memory of zero alignment");
    let buf = NonNull::new(buf).expect("Cannot free null pointer");
    allocator.free(buf, len, align);
}

unsafe extern "C" {
    unsafe static sy_defaultAllocator: *const c_void;

    pub fn sy_allocator_alloc(allocator: *const Allocator, len: usize, align: usize)
        -> *mut c_void;
    pub fn sy_allocator_free(
        allocator: *const Allocator,
        buf: *mut c_void,
        len: usize,
        align: usize,
    );
}

#[cfg(test)]
mod tests {
    use core::alloc::Layout;
    extern crate std;

    use super::*;

    #[test]
    fn sizes() {
        assert_eq!(size_of::<Allocator>(), size_of::<*const c_void>() * 2);
        assert_eq!(size_of::<AllocatorVTable>(), size_of::<*const c_void>() * 2);
    }

    struct TestAllocator;
    impl IAllocator for TestAllocator {
        fn alloc(self: &Self, len: NonZero<usize>, align: NonZero<usize>) -> *mut c_void {
            unsafe {
                let layout = Layout::from_size_align_unchecked(len.get(), align.get());
                return std::alloc::alloc(layout) as *mut c_void;
            }
        }

        fn free(self: &Self, buf: NonNull<c_void>, len: NonZero<usize>, align: NonZero<usize>) {
            unsafe {
                let layout = Layout::from_size_align_unchecked(len.get(), align.get());
                std::alloc::dealloc(buf.as_ptr() as *mut u8, layout);
            }
        }
    }

    #[test]
    fn iallocator_impl() {
        let test_alloc = TestAllocator;
        let allocator = test_alloc.allocator();
        let mut value = allocator
            .alloc_array::<u8>(NonZero::new(13).unwrap())
            .unwrap();
        let allocated_slice: &mut [u8] = unsafe { &mut value.as_mut() };
        unsafe { std::ptr::copy("Hello, world!".as_ptr(), allocated_slice.as_mut_ptr(), 13) };
        let s = std::str::from_utf8(&allocated_slice).expect("Should be valid utf8");
        assert_eq!(s, "Hello, world!");
    }
}
