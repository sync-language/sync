/// [C Tagged Unions](https://github.com/rust-lang/rfcs/blob/master/text/2195-really-tagged-unions.md)
#[repr(C, i32)]
pub enum Type {
    Bool(),
}
