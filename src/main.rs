use reqwest::{self, blocking::get};
use std::env;
use std::fs::{self, File};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

mod glb;

const CONFIG_URL: &str = "https://gist.githubusercontent.com/tylertms/7592bcbdd1b6891bdf9b2d1a4216b55b/raw/";
const DLC_URL: &str = "https://auxbrain.com/dlc/shells/";

fn print_usage(exec: &str) {
    eprintln!(
        "Usage: {} <input.rpo(z)?> [-s|--search <term>] [-o|--output <output>]",
        exec
    );
}
fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        print_usage(&args[0]);
        std::process::exit(1);
    }

    let mut input_path = None;
    let mut output_path = None;
    let mut search_term = None;

    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "-o" | "--output" => {
                if i + 1 < args.len() {
                    output_path = Some(args[i + 1].clone());
                    i += 1;
                } else {
                    eprintln!("error: missing output path");
                    print_usage(&args[0]);
                    std::process::exit(1);
                }
            }
            "-s" | "--search" => {
                if i + 1 < args.len() {
                    search_term = Some(args[i + 1].clone());
                } else {
                    eprintln!("error: missing search term");
                    print_usage(&args[0]);
                    std::process::exit(1);
                }
            }
            _ => {
                if input_path.is_none() {
                    input_path = Some(args[i].clone());
                } else {
                    eprintln!("error: multiple input paths provided");
                    print_usage(&args[0]);
                    std::process::exit(1);
                }
            }
        }
        i += 1;
    }

    if search_term.is_some() {
        let output_path = output_path.unwrap_or_else(|| ".".to_string());
        browse_shells(&search_term.unwrap(), Path::new(&output_path));
        return;
    }

    let input_path = input_path.expect("error: missing input path");
    let input_metadata = fs::metadata(&input_path).expect("error: failed to read input metadata");

    if input_metadata.is_dir() {
        if output_path.is_none() {
            output_path = Some(input_path.clone());
        }

        let output_path = output_path.unwrap();
        let output_metadata = fs::metadata(&output_path);
        if output_metadata.is_err() || !output_metadata.as_ref().unwrap().is_dir() {
            if output_metadata.is_ok() {
                eprintln!("error: {} is an existing file", output_path);
                std::process::exit(1);
            }
            fs::create_dir_all(&output_path).expect("error: failed to create output directory");
        }

        for entry in fs::read_dir(&input_path).expect("error: failed to read input directory") {
            let entry = entry.expect("error: failed to read directory entry");
            let path = entry.path();
            if path.is_file() {
                match path.extension().and_then(|ext| ext.to_str()) {
                    Some("rpo") | Some("rpoz") => {
                        let output_file = Path::new(&output_path)
                            .join(path.file_name().expect("error: invalid file name"))
                            .with_extension("glb");

                        convert_file(&path, &output_file);
                    }
                    _ => {}
                }
            }
        }
    } else {
        let output_path = output_path.unwrap_or_else(|| {
            let input_file_name = Path::new(&input_path)
                .file_stem()
                .expect("error: invalid input path")
                .to_string_lossy()
                .into_owned();
            format!("{}.glb", input_file_name)
        });

        let output_path = if output_path.ends_with(".glb") {
            output_path
        } else {
            format!("{}.glb", output_path)
        };

        let output_file = Path::new(&output_path);
        convert_file(&PathBuf::from(input_path), &output_file);
    }
}

fn convert_file(input_path: &Path, output_path: &Path) {
    let mut file_content = Vec::new();
    File::open(input_path)
        .unwrap()
        .read_to_end(&mut file_content)
        .unwrap();

        //get output path everything after last \ or / and before extension
    let glb = glb::convert_buffer(
        file_content,
        output_path.file_name().unwrap().to_str().unwrap().split('.').next().unwrap(),
    );
    let mut output_file = File::create(output_path).expect("error: failed to create output file");
    glb.to_writer(&mut output_file)
        .expect("error: failed to write glb to file");

    println!("{} -> {}", input_path.display(), output_path.display());
}

fn browse_shells(search_term: &str, output_path: &Path) {
    let config_response = get(CONFIG_URL).unwrap();
    let config = config_response.text().unwrap();

    let mut reader = csv::ReaderBuilder::new()
        .delimiter(b'|')
        .from_reader(config.as_bytes());

    let mut shells = Vec::new();
    let mut chickens = Vec::new();

    for result in reader.records() {
        let record = result.unwrap();
        let asset_type = record.get(0).unwrap().to_string();

        if record.into_iter().any(|field| field.contains(search_term)) {
            match asset_type.as_str() {
                "shell" => {
                    shells.push(record);
                }
                "chicken" => {
                    chickens.push(record);
                }
                _ => {}
            }
        }
    }

    if shells.is_empty() && chickens.is_empty() {
        println!(
            "No shells or chickens found for search term '{}'",
            search_term
        );
        return;
    }

    let shell_len = shells
        .iter()
        .map(|shell| shell.get(1).unwrap().len())
        .max()
        .unwrap_or(0)
        + 1;
    let shell_id_len = shells
        .iter()
        .map(|shell| shell.get(2).unwrap().len())
        .max()
        .unwrap_or(0);
    let chicken_len = chickens
        .iter()
        .map(|chicken| chicken.get(1).unwrap().len())
        .max()
        .unwrap_or(0)
        + 1;
    let chicken_id_len = chickens
        .iter()
        .map(|chicken| chicken.get(2).unwrap().len())
        .max()
        .unwrap_or(0);

    if !shells.is_empty() {
        println!();
        let header = format!(
            "{:name_len$} | {:>9} | {:id_len$}",
            "Shell",
            "Size",
            "ID",
            name_len = shell_len,
            id_len = shell_id_len
        );
        println!("{}", header);
        println!("{}", "-".repeat(header.len()));

        for shell in &shells {
            let name = shell.get(1).unwrap();
            let id = shell.get(2).unwrap();
            let size = bytes_to_readable(shell.get(4).unwrap().parse().unwrap());

            println!(
                "{:name_len$} | {:>9} | {:id_len$}",
                name,
                size,
                id,
                name_len = shell_len,
                id_len = shell_id_len
            );
        }
    }

    if !chickens.is_empty() {
        println!();
        let header = format!(
            "{:name_len$} | {:>9} | {:id_len$}",
            "Chicken",
            "Size",
            "ID",
            name_len = chicken_len,
            id_len = chicken_id_len
        );
        println!("{}", header);
        println!("{}", "-".repeat(header.len()));

        for chicken in &chickens {
            let name = chicken.get(1).unwrap();
            let id = chicken.get(2).unwrap();
            let size = bytes_to_readable(chicken.get(4).unwrap().parse().unwrap());

            println!(
                "{:name_len$} | {:>9} | {:id_len$}",
                name,
                size,
                id,
                name_len = chicken_len,
                id_len = chicken_id_len
            );
        }
    }

    let total_shell_size: u64 = shells
        .clone()
        .iter()
        .map(|shell| shell.get(4).unwrap().parse::<u64>().unwrap())
        .sum();
    let total_chicken_size: u64 = chickens
        .clone()
        .iter()
        .map(|chicken| chicken.get(4).unwrap().parse::<u64>().unwrap())
        .sum();

    println!("\n{:<20} | {:>9}", "Summary", "Size");
    println!("{}", "-".repeat(32));
    println!(
        "{:<20} | {:>9}",
        format!("{} Shells", shells.len()),
        bytes_to_readable(total_shell_size)
    );
    println!(
        "{:<20} | {:>9}\n",
        format!("{} Chickens", chickens.len()),
        bytes_to_readable(total_chicken_size)
    );

    println!(
        "Total Size: {}",
        bytes_to_readable(total_shell_size + total_chicken_size)
    );

    if output_path == Path::new(".") {
        print!("Download all to your current folder? (y/n) ");
    } else {
        print!("Download all to '{}'? (y/n) ", output_path.display());
    };
    let _ = std::io::stdout().flush();

    let mut input = String::new();
    std::io::stdin().read_line(&mut input).unwrap();

    if input.trim().to_lowercase() == "y" {
        if !output_path.exists() {
            fs::create_dir_all(output_path).expect("error: failed to create output directory");
        }

        let mut assets = Vec::new();
        assets.extend(shells);
        assets.extend(chickens);

        for asset in assets {
            let id = asset.get(2).unwrap();
            let hash = asset.get(3).unwrap();
            let url = format!("{}{}_{}.rpoz", DLC_URL, id, hash);

            let response = get(&url).unwrap();
            let glb = glb::convert_buffer(
                response.bytes().unwrap().to_vec(),
                id,
            );

            let output_file_path = output_path.join(format!("{}.glb", id));
            let mut output_file =
                File::create(&output_file_path).expect("error: failed to create output file");
            glb.to_writer(&mut output_file)
                .expect("error: failed to write glb to file");

            println!(
                "{} -> {}",
                url.split("/").last().unwrap(),
                &output_file_path.display()
            );
        }
    }
}

fn bytes_to_readable(bytes: u64) -> String {
    let units = ["B", "KB", "MB", "GB", "TB", "PB", "EB"];
    let mut bytes = bytes as f64;
    let mut i = 0;
    while bytes >= 1024.0 && i < units.len() - 1 {
        bytes /= 1024.0;
        i += 1;
    }
    format!("{:.2} {}", bytes, units[i])
}
