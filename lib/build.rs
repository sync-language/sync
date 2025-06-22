extern crate cmake;

fn main()
{
    let dst = cmake::Config::new(".")
        .no_build_target(true)
        .always_configure(true)
        .build();

    if cfg!(target_os = "macos") {
        println!("cargo:rustc-flags=-l dylib=c++");    
    }   
      
    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=static=SyncLib");
}
