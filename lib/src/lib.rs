mod mem;
mod types;

use mem::allocator;
use types::type_info;

pub use allocator::Allocator;
pub use allocator::AllocError;
pub use allocator::IAllocator;
pub use type_info::Type;

// pub mod c {
//     use super::mem::allocator;
//     pub use allocator::SyAllocator;
//     pub use allocator::SyAllocatorVTable;
//     pub use allocator::sy_allocator_alloc;
//     pub use allocator::sy_allocator_free;
//     pub use allocator::sy_allocator_destructor;
//     pub use allocator::sy_default_allocator;
// }
