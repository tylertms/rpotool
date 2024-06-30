# This is a Blender import script for importing converted Egg, Inc. .glb files
# To install, open Blender, go to Edit > Preferences > Add-ons > Install... and select this file
# Then click the checkbox to enable the add-on

# To import the converted .glb files, go to File > Import > Egg, Inc. glTF (.glb)
# If you use the default .gltf/.glb importer, you will not get emission or cycles backface culling

from bpy.props import (
    CollectionProperty,
    StringProperty,
)
from bpy.types import (
    Operator,
    OperatorFileListElement,
)
from bpy_extras.io_utils import ImportHelper
import os
import bpy
bl_info = {
    "name": "Egg, Inc. glTF Format",
    "blender": (4,1,0),
    "category": "Import-Export",
}


class EggIncGLTF(Operator, ImportHelper):
    bl_idname = "egg_inc_gltf.import_glb"
    bl_label = "Import Egg, Inc. glTF"
    filename_ext = ".glb"

    filter_glob: bpy.props.StringProperty(
        default="*.glb",
        options={'HIDDEN'},
        maxlen=255,
    )

    files: CollectionProperty(
        type=bpy.types.OperatorFileListElement,
        options={'HIDDEN'},
    )

    directory: StringProperty(
        maxlen=1024,
        subtype='DIR_PATH',
    )

    def execute(self, context):
        for file in self.files:
            file_path = os.path.join(self.directory, file.name)
            bpy.ops.import_scene.gltf(filepath=file_path)
            obj = context.object

            if obj.type == 'MESH':
                for slot in obj.material_slots:
                    material = slot.material
                    if material and material.use_nodes:
                        material.node_tree.links.clear()
                        nodes = material.node_tree.nodes

                        principled_bsdf = nodes.get('Principled BSDF')
                        principled_bsdf.location = (200, 0)

                        color_attribute = nodes.get('Color Attribute')
                        if not color_attribute:
                            color_attribute = nodes.new(
                                type='ShaderNodeVertexColor')
                        color_attribute.location = (-200, -100)

                        multiply = nodes.new(type='ShaderNodeMath')
                        multiply.operation = 'MULTIPLY'
                        multiply.inputs[1].default_value = 1.0
                        multiply.location = (0, -300)

                        material_output = nodes.get('Material Output')
                        material_output.location = (900, 0)

                        mix_shader = nodes.new(type='ShaderNodeMixShader')
                        mix_shader.location = (700, 0)

                        geometry = nodes.new(type='ShaderNodeNewGeometry')
                        geometry.location = (500, 200)

                        transparent_bsdf = nodes.new(
                            type='ShaderNodeBsdfTransparent')
                        transparent_bsdf.location = (500, -200)

                        links = material.node_tree.links

                        links.new(
                            color_attribute.outputs['Color'], principled_bsdf.inputs['Base Color'])
                        links.new(
                            color_attribute.outputs['Color'], principled_bsdf.inputs['Emission Color'])
                        links.new(
                            color_attribute.outputs['Alpha'], multiply.inputs[0])
                        links.new(
                            multiply.outputs['Value'], principled_bsdf.inputs['Emission Strength'])

                        links.new(
                            principled_bsdf.outputs['BSDF'], mix_shader.inputs[1])
                        links.new(
                            transparent_bsdf.outputs['BSDF'], mix_shader.inputs[2])
                        links.new(
                            geometry.outputs['Backfacing'], mix_shader.inputs['Fac'])
                        links.new(
                            mix_shader.outputs['Shader'], material_output.inputs['Surface'])

        return {'FINISHED'}


def menu_func_import(self, context):
    self.layout.operator(EggIncGLTF.bl_idname, text="Egg, Inc. glTF (.glb)")


def register():
    bpy.utils.register_class(EggIncGLTF)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.utils.unregister_class(EggIncGLTF)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)


if __name__ == "__main__":
    register()
