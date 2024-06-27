use std::fs::{self, File};
use std::io::{BufReader, Read, Seek, Write};
use std::path::{Path, PathBuf};

const MAGIC_HEADER: u32 = 0x52504F31;
const CHUNK_INDICATOR: u32 = 0x1406;

fn main() {
    // read arguments
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 4 || args[2] != "-o" {
        eprintln!("Usage: {} <input> -o <output>", args[0]);
        std::process::exit(1);
    }

    let input_path = &args[1];
    let output_path = &args[3];

    let input_metadata = fs::metadata(input_path).unwrap();

    if input_metadata.is_dir() {
        let output_metadata = fs::metadata(output_path);
        if output_metadata.is_err() || !output_metadata.as_ref().unwrap().is_dir() {
            if output_metadata.is_ok() {
                eprintln!("error: {} is an existing file", output_path);
                std::process::exit(1);
            }
            fs::create_dir_all(output_path).unwrap();
        }
        for entry in fs::read_dir(input_path).unwrap() {
            let entry = entry.unwrap();
            let path = entry.path();
            if path.is_file() {
                let output_file = Path::new(output_path).join(path.file_name().unwrap()).with_extension("obj");
                convert_file(&path, &output_file);
            } else {
                eprintln!("error: {} is not a file", path.display());
            }
        }
    } else {
        let output_file = Path::new(output_path).with_extension("obj");
        convert_file(&PathBuf::from(input_path), &output_file);
    }
}

fn convert_file(input_path: &Path, output_path: &Path) {
    // open file and check for valid RPO1 header
    let mut file = BufReader::new(File::open(input_path).unwrap());
    let mut quad_buffer = [0; 4];

    file.read_exact(&mut quad_buffer).unwrap();
    let header = u32::from_be_bytes(quad_buffer);

    if header != MAGIC_HEADER {
        eprintln!("error: {} is not a valid .rpo file", input_path.display());
        return;
    }

    // read vertex and face count
    file.read_exact(&mut quad_buffer).unwrap();
    let vertices_count = u32::from_le_bytes(quad_buffer);

    file.read_exact(&mut quad_buffer).unwrap();
    let faces_count = u32::from_le_bytes(quad_buffer) / 6;

    //println!("Vertices: {}", vertices_count);
    //println!("Faces: {}", faces_count);

    let header_length: u32;
    let mut tokens: u8 = 0;
    let mut value: u32;

    // find header length and token count
    loop {
        file.read_exact(&mut quad_buffer).unwrap();
        value = u32::from_le_bytes(quad_buffer);

        if value == CHUNK_INDICATOR {
            file.seek_relative(-8).unwrap();
            file.read_exact(&mut quad_buffer).unwrap();
            file.seek_relative(4).unwrap();
            tokens += u32::from_le_bytes(quad_buffer) as u8;
            continue;
        }

        if value == 0 {
            file.read_exact(&mut quad_buffer).unwrap();
            value = u32::from_le_bytes(quad_buffer);
            if value > 4 {
                header_length = file.stream_position().unwrap() as u32;
                break;
            }
            file.seek_relative(-4).unwrap();
        }
    }


    file.seek(std::io::SeekFrom::Start(header_length as u64)).unwrap();

    // read vertices
    let mut vertices: Vec<(f32, f32, f32, f32, f32, f32, f32)> = Vec::with_capacity(vertices_count as usize);
    for _ in 0..vertices_count {
        let mut vertex = (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        for i in 0..tokens {
            file.read_exact(&mut quad_buffer).unwrap();
            let value = f32::from_le_bytes(quad_buffer);
            match i {
                0 => vertex.0 = value,
                1 => vertex.1 = value,
                2 => vertex.2 = value,
                3 => vertex.3 = value,
                4 => vertex.4 = value,
                5 => vertex.5 = value,
                6 => vertex.6 = value,
                _ => (),
            }
        }
        vertices.push(vertex);
    }

    // write vertices
    let mut out = File::create(output_path).unwrap();
    out.write_all("# Converted from .rpo\n".as_bytes()).unwrap();
    out.write_all("# This file is property of Auxbrain, Inc. and should be treated as such.\n\n".as_bytes()).unwrap();
    for vertex in vertices.iter() {
        out.write_all(format!("v {} {} {} {} {} {}\n", vertex.0, vertex.1, vertex.2, vertex.3, vertex.4, vertex.5).as_bytes()).unwrap();
    }

    // read faces
    let mut faces: Vec<(u16, u16, u16)> = Vec::with_capacity(faces_count as usize);
    let mut buffer: [u8; 2] = [0; 2];
    for _ in 0..faces_count {
        let mut face = (0, 0, 0);
        for i in 0..3 {
            file.read_exact(&mut buffer).unwrap();
            match i {
                0 => face.0 = u16::from_le_bytes(buffer),
                1 => face.1 = u16::from_le_bytes(buffer),
                2 => face.2 = u16::from_le_bytes(buffer),
                _ => (),
            }
        }
        faces.push(face);
    }

    // write faces
    for face in faces.iter() {
        out.write_all(format!("f {} {} {}\n", face.0 + 1, face.1 + 1, face.2 + 1).as_bytes()).unwrap();
    }

    //close file
    out.flush().unwrap();
    out.sync_all().unwrap();

    println!("{} -> {}", input_path.display(), output_path.display());
}
