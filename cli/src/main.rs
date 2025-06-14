extern crate sync_lang;
use std::{num::NonZero, str};

use sync_lang::Allocator;


fn main() {
    let mut alloc = Allocator::default();
    let mut value = alloc.alloc_array::<u8>(NonZero::new(13).unwrap()).unwrap();
    let allocated_slice: &mut [u8] = unsafe { &mut value.as_mut() };
    unsafe { std::ptr::copy("Hello, world!".as_ptr(), allocated_slice.as_mut_ptr(), 13) };
    println!("{}", str::from_utf8(allocated_slice).unwrap());
}
