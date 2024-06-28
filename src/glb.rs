use gltf_json as json;
use std::{fs, mem};

use json::validation::Checked::Valid;
use json::validation::USize64;
use std::borrow::Cow;
use std::io::Write;

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
enum Output {
    /// Output standard glTF.
    Standard,

    /// Output binary glTF.
    Binary,
}

#[derive(Copy, Clone, Debug)]
#[repr(C)]
struct Vertex {
    position: [f32; 3],
    color: [f32; 3],
}

/// Calculate bounding coordinates of a list of vertices, used for the clipping distance of the model
fn bounding_coords(points: &[Vertex]) -> ([f32; 3], [f32; 3]) {
    let mut min = [f32::MAX, f32::MAX, f32::MAX];
    let mut max = [f32::MIN, f32::MIN, f32::MIN];

    for point in points {
        let p = point.position;
        for i in 0..3 {
            min[i] = f32::min(min[i], p[i]);
            max[i] = f32::max(max[i], p[i]);
        }
    }
    (min, max)
}

fn align_to_multiple_of_four(n: &mut usize) {
    *n = (*n + 3) & !3;
}

fn to_padded_byte_vector(vertices: Vec<Vertex>, indices: Vec<u32>) -> Vec<u8> {
    let mut bytes: Vec<u8> = Vec::new();
    for vertex in vertices {
        bytes.extend_from_slice(&unsafe {
            std::mem::transmute::<[f32; 3], [u8; 12]>(vertex.position)
        });
        bytes
            .extend_from_slice(&unsafe { std::mem::transmute::<[f32; 3], [u8; 12]>(vertex.color) });
    }
    for index in indices {
        bytes.extend_from_slice(&index.to_le_bytes());
    }
    // Align to 4 bytes
    while bytes.len() % 4 != 0 {
        bytes.push(0);
    }
    bytes
}

pub fn export(vertices: Vec<[f32; 7]>) {
    let mut indices: Vec<Vec<u32>> = Vec::new();
    let mut current_indices: Vec<u32> = Vec::new();
    let mut current_value = vertices[0][6];
    
    for (i, v) in vertices.iter().enumerate() {
        if v[6] != current_value {
            indices.push(current_indices);
            current_indices = Vec::new();
            current_value = v[6];
        }
        current_indices.push(i as u32);
    }
    indices.push(current_indices);

    let triangle_vertices = vertices
        .iter()
        .map(|v| Vertex {
            position: [v[0], v[1], v[2]],
            color: [v[3], v[4], v[5]],
        })
        .collect::<Vec<Vertex>>();

    let (min, max) = bounding_coords(&triangle_vertices);

    let mut root = gltf_json::Root::default();

    // Calculate buffer sizes
    let vertex_buffer_length = triangle_vertices.len() * mem::size_of::<Vertex>();
    let index_buffer_lengths: Vec<usize> = indices.iter().map(|i| i.len() * mem::size_of::<u32>()).collect();
    let index_buffer_total_length: usize = index_buffer_lengths.iter().sum();
    let buffer_length = vertex_buffer_length + index_buffer_total_length;

    // Create buffer
    let buffer = root.push(json::Buffer {
        byte_length: USize64::from(buffer_length),
        extensions: Default::default(),
        extras: Default::default(),
        name: None,
        uri: None,
    });

    // Create buffer views
    let vertex_buffer_view = root.push(json::buffer::View {
        buffer,
        byte_length: USize64::from(vertex_buffer_length),
        byte_offset: None,
        byte_stride: Some(json::buffer::Stride(mem::size_of::<Vertex>())),
        extensions: Default::default(),
        extras: Default::default(),
        name: None,
        target: Some(Valid(json::buffer::Target::ArrayBuffer)),
    });

    let index_buffer_views: Vec<_> = (0..indices.len())
        .map(|i| root.push(json::buffer::View {
            buffer,
            byte_length: USize64::from(indices[i].len() * mem::size_of::<u32>()),
            byte_offset: Some(USize64::from(
                vertex_buffer_length + indices[..i].iter().map(|ind| ind.len() * mem::size_of::<u32>()).sum::<usize>(),
            )),
            byte_stride: None,
            extensions: Default::default(),
            extras: Default::default(),
            name: None,
            target: Some(Valid(json::buffer::Target::ElementArrayBuffer)),
        }))
        .collect();

    // Create accessors
    let positions = root.push(json::Accessor {
        buffer_view: Some(vertex_buffer_view),
        byte_offset: Some(USize64(0)),
        count: USize64::from(triangle_vertices.len()),
        component_type: Valid(json::accessor::GenericComponentType(
            json::accessor::ComponentType::F32,
        )),
        extensions: Default::default(),
        extras: Default::default(),
        type_: Valid(json::accessor::Type::Vec3),
        min: Some(json::Value::from(Vec::from(min))),
        max: Some(json::Value::from(Vec::from(max))),
        name: None,
        normalized: false,
        sparse: None,
    });

    let colors = root.push(json::Accessor {
        buffer_view: Some(vertex_buffer_view),
        byte_offset: Some(USize64::from(3 * mem::size_of::<f32>())),
        count: USize64::from(triangle_vertices.len()),
        component_type: Valid(json::accessor::GenericComponentType(
            json::accessor::ComponentType::F32,
        )),
        extensions: Default::default(),
        extras: Default::default(),
        type_: Valid(json::accessor::Type::Vec3),
        min: None,
        max: None,
        name: None,
        normalized: false,
        sparse: None,
    });

    let index_accessors: Vec<_> = (0..indices.len())
        .map(|i| root.push(json::Accessor {
            buffer_view: Some(index_buffer_views[i]),
            byte_offset: Some(USize64(0)),
            count: USize64::from(indices[i].len()),
            component_type: Valid(json::accessor::GenericComponentType(
                json::accessor::ComponentType::U32,
            )),
            extensions: Default::default(),
            extras: Default::default(),
            type_: Valid(json::accessor::Type::Scalar),
            min: None,
            max: None,
            name: None,
            normalized: false,
            sparse: None,
        }))
        .collect();

    let materials: Vec<_> = vec![
        ("default", [1.0, 1.0, 1.0, 1.0], [0.0, 0.0, 0.0]),
        ("emissive", [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 0.0]),
        ("emissive2", [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 0.0]),
        ("emissive3", [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 0.0]),
        ("emissive4", [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 0.0]),
        ("emissive5", [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 0.0]),
        ("emissive6", [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 0.0]),
    ]
    .into_iter()
    .map(|(name, base_color, emissive_factor)| {
        root.push(json::Material {
            name: Some(name.into()),
            pbr_metallic_roughness: json::material::PbrMetallicRoughness {
                base_color_factor: gltf_json::material::PbrBaseColorFactor(base_color),
                metallic_factor: gltf_json::material::StrengthFactor(1.0),
                roughness_factor: gltf_json::material::StrengthFactor(1.0),
                base_color_texture: None,
                metallic_roughness_texture: None,
                extensions: Default::default(),
                extras: Default::default(),
            },
            normal_texture: None,
            occlusion_texture: None,
            emissive_texture: None,
            emissive_factor: gltf_json::material::EmissiveFactor(emissive_factor),
            alpha_mode: Valid(json::material::AlphaMode::Opaque),
            alpha_cutoff: None,
            double_sided: false,
            extensions: Default::default(),
            extras: Default::default(),
        })
    })
    .collect();

    let primitives: Vec<_> = (0..indices.len())
        .map(|i| json::mesh::Primitive {
            attributes: {
                let mut map = std::collections::BTreeMap::new();
                map.insert(Valid(json::mesh::Semantic::Positions), positions);
                map.insert(Valid(json::mesh::Semantic::Colors(0)), colors);
                map
            },
            indices: Some(index_accessors[i]),
            material: Some(materials[i]),
            mode: Valid(json::mesh::Mode::Triangles),
            targets: None,
            extensions: Default::default(),
            extras: Default::default(),
        })
        .collect();

    let mesh = root.push(json::Mesh {
        extensions: Default::default(),
        extras: Default::default(),
        name: None,
        primitives,
        weights: None,
    });

    let node = root.push(json::Node {
        mesh: Some(mesh),
        ..Default::default()
    });

    root.push(json::Scene {
        extensions: Default::default(),
        extras: Default::default(),
        name: None,
        nodes: vec![node],
    });

    let json_string = json::serialize::to_string(&root).expect("Serialization error");
    let mut json_offset = json_string.len();
    align_to_multiple_of_four(&mut json_offset);
    let glb = gltf::binary::Glb {
        header: gltf::binary::Header {
            magic: *b"glTF",
            version: 2,
            length: (json_offset + buffer_length)
                .try_into()
                .expect("file size exceeds binary glTF limit"),
        },
        bin: Some(Cow::Owned(to_padded_byte_vector(
            triangle_vertices,
            indices.into_iter().flatten().collect(),
        ))),
        json: Cow::Owned(json_string.into_bytes()),
    };
    let writer = fs::File::create("triangle.glb").expect("I/O error");
    glb.to_writer(writer).expect("glTF binary output error");
}

