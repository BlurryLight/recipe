import math
import os
from .model import Scene, BSDF_MAP, TransformOp, INTEGRATOR_MAP, SAMPLER_MAP
from .obj_reader import read_obj


def _vec3_str(v):
    return " ".join(str(x) for x in v)


def _nori_hfov_to_pbrt_vfov(hfov_deg: float, width: int, height: int) -> float:
    aspect = width / float(height)
    hfov_rad = math.radians(hfov_deg)
    vfov_rad = 2.0 * math.atan(math.tan(hfov_rad / 2.0) / aspect)
    return math.degrees(vfov_rad)


def _find_lookat(transform_ops: list[TransformOp]) -> TransformOp | None:
    for op in transform_ops:
        if op.op_type == "lookat":
            return op
    return None


def _find_scale(transform_ops: list[TransformOp]) -> TransformOp | None:
    for op in transform_ops:
        if op.op_type == "scale":
            return op
    return None


def _find_rotate(transform_ops: list[TransformOp]) -> TransformOp | None:
    for op in transform_ops:
        if op.op_type == "rotate":
            return op
    return None


def _emit_pbrt_transform(lines: list[str], ops: list[TransformOp],
                          indent: str = ""):
    for op in ops:
        if op.op_type == "scale":
            lines.append(f"{indent}Scale {_vec3_str(op.values)}")
        elif op.op_type == "translate":
            lines.append(f"{indent}Translate {_vec3_str(op.values)}")
        elif op.op_type == "rotate":
            axis_str = _vec3_str(op.axis) if op.axis else "0 0 1"
            lines.append(f"{indent}Rotate {op.angle} {axis_str}")
        elif op.op_type == "lookat":
            lines.append(
                f"{indent}LookAt {_vec3_str(op.origin)}  "
                f"{_vec3_str(op.target)}  {_vec3_str(op.up)}"
            )
        elif op.op_type == "matrix":
            lines.append(f"{indent}Transform [{_vec3_str(op.values)}]")


def _emit_material(lines: list[str], bsdf):
    mapping = BSDF_MAP.get(bsdf.type)
    if mapping is None:
        lines.append(f"# Warning: unknown BSDF type '{bsdf.type}', using matte")
        mapping = BSDF_MAP["diffuse"]

    material_name = mapping["material"]
    param_args = []

    for pbrt_param, (pbrt_type, nori_name, default_val) in mapping["params"].items():
        if nori_name and nori_name in bsdf.params:
            val = bsdf.params[nori_name]
        else:
            val = default_val

        if isinstance(val, list):
            val_str = f"[{_vec3_str(val)}]"
        else:
            val_str = str(val)

        param_args.append(f'"{"color" if pbrt_type == "rgb" or pbrt_type == "color" else pbrt_type} {pbrt_param}" {val_str}')

    lines.append(f'Material "{material_name}" ' + " ".join(param_args))


def _emit_obj_as_trianglemesh(lines: list[str], obj_path: str,
                                indent: str = ""):
    mesh = read_obj(obj_path)
    if mesh.is_empty():
        lines.append(f"{indent}# Warning: could not load {obj_path}")
        return

    lines.append(f'{indent}Shape "trianglemesh"')

    if mesh.positions:
        lines.append(f'{indent}  "point3 P" [')
        for p in mesh.positions:
            lines.append(f"{indent}    {p[0]:.10g} {p[1]:.10g} {p[2]:.10g}")
        lines.append(f"{indent}  ]")

    if mesh.normals:
        lines.append(f'{indent}  "normal N" [')
        for n in mesh.normals:
            lines.append(f"{indent}    {n[0]:.10g} {n[1]:.10g} {n[2]:.10g}")
        lines.append(f"{indent}  ]")

    if mesh.texcoords:
        lines.append(f'{indent}  "point2 st" [')
        for st in mesh.texcoords:
            lines.append(f"{indent}    {st[0]:.10g} {st[1]:.10g}")
        lines.append(f"{indent}  ]")

    lines.append(f'{indent}  "integer indices" [')
    for i in range(0, len(mesh.indices), 3):
        i0, i1, i2 = mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]
        lines.append(f"{indent}    {i0} {i1} {i2}")
    lines.append(f"{indent}  ]")


def write_pbrt(scene: Scene, output_path: str):
    lines = []

    cam = scene.camera
    lookat = _find_lookat(cam.transform)
    scale = _find_scale(cam.transform)

    if lookat:
        if scale:
            flipped = list(scale.values)
            if len(flipped) == 1:
                flipped[0] = -flipped[0]
            elif len(flipped) >= 3:
                flipped[0] = -flipped[0]
            lines.append(f"Scale {_vec3_str(flipped)}")
        lines.append(
            f"LookAt {_vec3_str(lookat.origin)}  "
            f"{_vec3_str(lookat.target)}  {_vec3_str(lookat.up)}"
        )
    else:
        lines.append("LookAt 0 0 10  0 0 0  0 1 0")

    lines.append('Camera "perspective"')
    lines.append(f'  "float fov" [ {cam.fov:.6g} ]')
    lines.append('Film "image"')
    lines.append(f'  "integer xresolution" [ {cam.width} ]')
    lines.append(f'  "integer yresolution" [ {cam.height} ]')
    lines.append(f'  "string filename" "output.exr"')
    pbrt_sampler = SAMPLER_MAP.get(scene.sampler.type, scene.sampler.type)
    lines.append(f'Sampler "{pbrt_sampler}"')
    lines.append(f'  "integer pixelsamples" [ {scene.sampler.sample_count} ]')

    pbrt_int = INTEGRATOR_MAP.get(scene.integrator)
    if pbrt_int:
        lines.append(f'Integrator "{pbrt_int}"')
    else:
        lines.append(f'Integrator "path"')

    lines.append("WorldBegin")
    lines.append("")

    for emitter in scene.emitters:
        if emitter.type == "point":
            pos = emitter.params.get("position", [0, 0, 0])
            power = emitter.params.get("power", [1, 1, 1])
            lines.append(f'AttributeBegin')
            lines.append(f'  LightSource "point" '
                         f'"point3 from" [{_vec3_str(pos)}] '
                         f'"color I" [{_vec3_str(power)}]')
            lines.append(f'AttributeEnd')
            lines.append("")

    for mesh in scene.meshes:
        lines.append("AttributeBegin")

        obj_transform_ops = [op for op in mesh.transform
                               if op.op_type != "lookat"]
        _emit_pbrt_transform(lines, obj_transform_ops, indent="  ")

        if mesh.emitter:
            radiance = mesh.emitter.params.get("radiance", [1, 1, 1])
            lines.append(f'  AreaLightSource "area" '
                         f'"color L" [{_vec3_str(radiance)}]')

        if mesh.bsdf:
            _emit_material(lines, mesh.bsdf)
        elif mesh.emitter:
            lines.append(f'  Material "matte" "color Kd" [0 0 0]')
        else:
            lines.append(f'  Material "matte" "color Kd" [0.5 0.5 0.5]')

        obj_path = mesh.obj_filename
        obj_basename = os.path.basename(obj_path)
        lines.append(f"  # Mesh: {obj_basename}")
        _emit_obj_as_trianglemesh(lines, obj_path, indent="  ")

        lines.append("AttributeEnd")
        lines.append("")

    lines.append("WorldEnd")

    with open(output_path, "w") as f:
        f.write("\n".join(lines) + "\n")
