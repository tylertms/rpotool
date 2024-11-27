use flate2::read::DeflateDecoder;
use gltf_json as json;
use json::validation::Checked::Valid;
use json::validation::USize64;
use std::borrow::Cow;
use std::io::Read;
use std::process::exit;

const RPO1_MAGIC: u32 = 0x52504F31;
const RPOZ_MAGIC: u16 = 0x789C;

pub fn convert_buffer(rpo_data: Vec<u8>, name: &str) -> gltf::binary::Glb<'static> {
    let mut bin = if rpo_data.starts_with(&RPO1_MAGIC.to_be_bytes()) {
        rpo_data
    } else if rpo_data.starts_with(&RPOZ_MAGIC.to_be_bytes()) {
        let compressed_data = &rpo_data[2..];
        let mut decoder = DeflateDecoder::new(compressed_data);
        let mut buffer = Vec::new();
        decoder.read_to_end(&mut buffer).unwrap();
        buffer
    } else {
        eprintln!("error: provided buffer is not a valid .rpo(z) file");
        exit(1);
    };

    let vertex_count_buf = bin[4..8].try_into().expect("Invalid byte slice length");
    let face_bytes_buf = bin[8..12].try_into().expect("Inavlid byte slice length");

    let vertex_count = u32::from_le_bytes(vertex_count_buf);
    let face_bytes = u32::from_le_bytes(face_bytes_buf);

    let mut strides = Vec::new();

    while u32::from_le_bytes(bin[0..4].try_into().unwrap()) != face_bytes / 2 {
        if bin.len() < 8 {
            panic!("error: RPO file was invalid, could not find end of header");
        }

        if bin[4..8] == [0x06, 0x14, 0x00, 0x00] {
            strides.push(bin[0]);
        }

        bin = bin.split_off(4);
    }

    bin = bin.split_off(4);
    let buffer_len = bin.len();
    let stride_len = strides.iter().sum::<u8>() as usize;

    let mut root = json::Root::default();

    let buffer = root.push(json::Buffer {
        byte_length: USize64(buffer_len as u64),
        uri: None,
        name: None,
        extensions: Default::default(),
        extras: Default::default(),
    });

    let mut offset = 0;
    let mut buffer_views = Vec::new();
    for stride in &strides {
        buffer_views.push(root.push(json::buffer::View {
            buffer,
            byte_offset: Some(USize64(offset)),
            byte_length: USize64(vertex_count as u64 * stride_len as u64 * 4),
            byte_stride: Some(json::buffer::Stride(stride_len * 4)),
            target: None,
            name: None,
            extensions: Default::default(),
            extras: Default::default(),
        }));
        offset += *stride as u64 * 4;
    }

    buffer_views.push(root.push(json::buffer::View {
        buffer,
        byte_offset: Some(USize64(vertex_count as u64 * stride_len as u64 * 4)),
        byte_length: USize64(face_bytes as u64),
        byte_stride: None,
        target: None,
        name: None,
        extensions: Default::default(),
        extras: Default::default(),
    }));

    let mut accessors = Vec::new();
    for (i, stride) in strides.iter().enumerate() {
        let accessor_type;
        match stride {
            2 => accessor_type = json::accessor::Type::Vec2,
            3 => accessor_type = json::accessor::Type::Vec3,
            4 => accessor_type = json::accessor::Type::Vec4,
            _ => accessor_type = json::accessor::Type::Scalar,
        }
        accessors.push(root.push(json::Accessor {
            buffer_view: Some(buffer_views[i]),
            byte_offset: None,
            component_type: Valid(json::accessor::GenericComponentType(
                json::accessor::ComponentType::F32,
            )),
            count: USize64(vertex_count as u64),
            type_: Valid(accessor_type),
            min: None,
            max: None,
            normalized: false,
            sparse: None,
            name: None,
            extensions: Default::default(),
            extras: Default::default(),
        }));
    }

    accessors.push(root.push(json::Accessor {
        buffer_view: Some(buffer_views[strides.len()]),
        byte_offset: None,
        component_type: Valid(json::accessor::GenericComponentType(
            json::accessor::ComponentType::U16,
        )),
        count: USize64(face_bytes as u64 / 2),
        type_: Valid(json::accessor::Type::Scalar),
        min: None,
        max: None,
        normalized: false,
        sparse: None,
        name: None,
        extensions: Default::default(),
        extras: Default::default(),
    }));

    let material = root.push(json::Material {
        name: Some("default".to_string()),
        extensions: Default::default(),
        extras: Default::default(),
        pbr_metallic_roughness: json::material::PbrMetallicRoughness {
            base_color_factor: json::material::PbrBaseColorFactor([1.0, 1.0, 1.0, 1.0]),
            metallic_factor: json::material::StrengthFactor(0.5),
            roughness_factor: json::material::StrengthFactor(0.5),
            base_color_texture: None,
            metallic_roughness_texture: None,
            extensions: Default::default(),
            extras: Default::default(),
        },
        normal_texture: None,
        occlusion_texture: None,
        emissive_texture: None,
        emissive_factor: json::material::EmissiveFactor([0.0, 0.0, 0.0]),
        alpha_mode: Valid(json::material::AlphaMode::Opaque),
        alpha_cutoff: None,
        double_sided: false,
    });    

    let primitive = json::mesh::Primitive {
        attributes: {
            let mut map = std::collections::BTreeMap::new();
            map.insert(Valid(json::mesh::Semantic::Positions), accessors[0]);
            if accessors.len() > 2 && *strides.get(1).unwrap() >= 3 {
                map.insert(Valid(json::mesh::Semantic::Colors(0)), accessors[1]);
            }
            if accessors.len() > 3 && *strides.get(2).unwrap() == 3 {
                map.insert(Valid(json::mesh::Semantic::Normals), accessors[2]);
            }
            map
        },
        indices: Some(accessors[accessors.len() - 1]),
        material: Some(material),
        mode: Valid(json::mesh::Mode::Triangles),
        targets: None,
        extensions: Default::default(),
        extras: Default::default(),
    };

    let mesh = root.push(json::Mesh {
        primitives: vec![primitive],
        weights: None,
        name: Some(name.to_string()),
        extensions: Default::default(),
        extras: Default::default(),
    });

    let node = root.push(json::Node {
        camera: None,
        children: None,
        skin: None,
        matrix: None,
        mesh: Some(mesh),
        rotation: None,
        scale: None,
        translation: None,
        weights: None,
        name: None,
        extensions: Default::default(),
        extras: Default::default(),
    });

    root.push(json::Scene {
        nodes: vec![node],
        name: None,
        extensions: Default::default(),
        extras: Default::default(),
    });

    let json_string = json::serialize::to_string(&root).expect("Serialization error");
    let mut json_offset = json_string.len();
    json_offset = (json_offset + 3) & !3;

    let glb = gltf::binary::Glb {
        header: gltf::binary::Header {
            magic: *b"glTF",
            version: 2,
            length: (json_offset + buffer_len)
                .try_into()
                .expect("file size exceeds binary glTF limit"),
        },
        bin: Some(Cow::Owned(bin)),
        json: Cow::Owned(json_string.into_bytes()),
    };

    return glb;
}
