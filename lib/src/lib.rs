extern crate core;
use core::ffi::c_int;

unsafe extern "C" {
    fn test_add(a: c_int, b: c_int) -> c_int;
}

pub fn do_add(a: i32, b: i32) -> i32 {
    unsafe {return test_add(a as c_int, b as c_int) as i32};
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn some_test() {
        assert_eq!(do_add(4, 5), 9);
    }
}