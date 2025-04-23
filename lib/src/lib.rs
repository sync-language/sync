mod mem;

use mem::allocator;

pub use allocator::Allocator;
pub use allocator::AllocError;
pub use allocator::IAllocator;

pub mod c {
    use super::mem::allocator;
    pub use allocator::SyAllocator;
    pub use allocator::SyAllocatorVTable;
    pub use allocator::sy_allocator_alloc;
    pub use allocator::sy_allocator_free;
    pub use allocator::sy_allocator_destructor;
    pub use allocator::sy_default_allocator;
}
