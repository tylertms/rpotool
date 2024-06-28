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
    let triangle_vertices = vertices
        .iter()
        .map(|v| Vertex {
            position: [v[0], v[1], v[2]],
            color: [v[3], v[4], v[5]],
        })
        .collect::<Vec<Vertex>>();

    let triangle_indices_first = // 0-indexed
        (0..(vertices.len() / 2) as u32)
            .collect::<Vec<u32>>();
    let triangle_indices_second = // 0-indexed
        ((vertices.len() / 2) as u32..vertices.len() as u32)
            .collect::<Vec<u32>>();

    let (min, max) = bounding_coords(&triangle_vertices);

    let mut root = gltf_json::Root::default();

    // Calculate buffer sizes
    let vertex_buffer_length = triangle_vertices.len() * mem::size_of::<Vertex>();
    let index_buffer_first_length = triangle_indices_first.len() * mem::size_of::<u32>();
    let index_buffer_second_length = triangle_indices_second.len() * mem::size_of::<u32>();
    let buffer_length = vertex_buffer_length + index_buffer_first_length + index_buffer_second_length;

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

    let index_buffer_first_view = root.push(json::buffer::View {
        buffer,
        byte_length: USize64::from(index_buffer_first_length),
        byte_offset: Some(USize64::from(vertex_buffer_length)),
        byte_stride: None,
        extensions: Default::default(),
        extras: Default::default(),
        name: None,
        target: Some(Valid(json::buffer::Target::ElementArrayBuffer)),
    });

    let index_buffer_second_view = root.push(json::buffer::View {
        buffer,
        byte_length: USize64::from(index_buffer_second_length),
        byte_offset: Some(USize64::from(vertex_buffer_length + index_buffer_first_length)),
        byte_stride: None,
        extensions: Default::default(),
        extras: Default::default(),
        name: None,
        target: Some(Valid(json::buffer::Target::ElementArrayBuffer)),
    });

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

    let indices_first = root.push(json::Accessor {
        buffer_view: Some(index_buffer_first_view),
        byte_offset: Some(USize64(0)),
        count: USize64::from(triangle_indices_first.len()),
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
    });

    let indices_second = root.push(json::Accessor {
        buffer_view: Some(index_buffer_second_view),
        byte_offset: Some(USize64(0)),
        count: USize64::from(triangle_indices_second.len()),
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
    });

    let material = root.push(json::Material {
        name: Some("default".into()),
        pbr_metallic_roughness: json::material::PbrMetallicRoughness {
            base_color_factor: gltf_json::material::PbrBaseColorFactor([1.0, 1.0, 1.0, 1.0]),
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
        emissive_factor: gltf_json::material::EmissiveFactor([0.0, 0.0, 0.0]),
        alpha_mode: Valid(json::material::AlphaMode::Opaque),
        alpha_cutoff: None,
        double_sided: false,
        extensions: Default::default(),
        extras: Default::default(),
    });

    let material_emissive = root.push(json::Material {
        name: Some("emissive".into()),
        pbr_metallic_roughness: json::material::PbrMetallicRoughness {
            base_color_factor: gltf_json::material::PbrBaseColorFactor([1.0, 1.0, 1.0, 1.0]),
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
        emissive_factor: gltf_json::material::EmissiveFactor([1.0, 1.0, 0.0]),
        alpha_mode: Valid(json::material::AlphaMode::Opaque),
        alpha_cutoff: None,
        double_sided: false,
        extensions: Default::default(),
        extras: Default::default(),
    });

    let primitive_first = json::mesh::Primitive {
        attributes: {
            let mut map = std::collections::BTreeMap::new();
            map.insert(Valid(json::mesh::Semantic::Positions), positions);
            map.insert(Valid(json::mesh::Semantic::Colors(0)), colors);
            map
        },
        indices: Some(indices_first),
        material: Some(material),
        mode: Valid(json::mesh::Mode::Triangles),
        targets: None,
        extensions: Default::default(),
        extras: Default::default(),
    };

    let primitive_second = json::mesh::Primitive {
        attributes: {
            let mut map = std::collections::BTreeMap::new();
            map.insert(Valid(json::mesh::Semantic::Positions), positions);
            map.insert(Valid(json::mesh::Semantic::Colors(0)), colors);
            map
        },
        indices: Some(indices_second),
        material: Some(material_emissive),
        mode: Valid(json::mesh::Mode::Triangles),
        targets: None,
        extensions: Default::default(),
        extras: Default::default(),
    };

    let mesh = root.push(json::Mesh {
        extensions: Default::default(),
        extras: Default::default(),
        name: None,
        primitives: vec![primitive_first, primitive_second],
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
            {
                let mut indices = triangle_indices_first;
                indices.extend(triangle_indices_second);
                indices
            }
        ))),
        json: Cow::Owned(json_string.into_bytes()),
    };
    let writer = fs::File::create("triangle.glb").expect("I/O error");
    glb.to_writer(writer).expect("glTF binary output error");
}
