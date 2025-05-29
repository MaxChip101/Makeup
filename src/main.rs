use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    println!("{}", args[0]);
    println!("Hello, world!");
}
