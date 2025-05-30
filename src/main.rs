use std::env;

fn help() {
    println!("I see you wanted some help.");
}

fn create_makeup_configs() {
    println!("creating");
}

fn makeup_config_details() {
    println!("details");
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        println!("Type 'makeup help' for a list of commands");
        return;
    }

    for arg in args {
        if arg == "--help" || arg == "-h" || arg == "help" {
            help();
            return;
        } else if arg == "new" {
            create_makeup_configs();
            return;
        } else if arg == "detail" {
            makeup_config_details();
            return;
        }
    }
}
