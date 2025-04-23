use sync_lang::Allocator;


fn main() {
    let mut alloc = Allocator::default();
    _ = alloc.alloc_object::<u8>();
    println!("Hello, world!");
}
