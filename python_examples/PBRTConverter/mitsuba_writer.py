import os
import xml.etree.ElementTree as ET
from xml.dom import minidom
from .model import (
    Scene, BSDF, TransformOp,
    MITSUBA_BSDF_MAP, MITSUBA_INTEGRATOR_MAP, MITSUBA_SAMPLER_MAP,
)


def _vec3_str(v, sep=","):
    return sep.join(str(x) for x in v)


def _bsdf_signature(bsdf) -> str:
    sig = bsdf.type + ";"
    for k in sorted(bsdf.params.keys()):
        v = bsdf.params[k]
        if isinstance(v, list):
            sig += f"{k}=[{_vec3_str(v, ' ')}];"
        else:
            sig += f"{k}={v};"
    return sig


def _make_default_diffuse() -> BSDF:
    return BSDF(type="diffuse", params={"albedo": [0.5, 0.5, 0.5]})


def _make_black_diffuse() -> BSDF:
    return BSDF(type="diffuse", params={"albedo": [0.0, 0.0, 0.0]})


def _assign_bsdf_ids(scene: Scene):
    sig_to_id = {}
    bsdf_map = {}
    counter = 0

    for mesh in scene.meshes:
        bsdf = mesh.bsdf
        if bsdf is None:
            # Emissive meshes get black diffuse, others get default
            if mesh.emitter:
                bsdf = _make_black_diffuse()
            else:
                bsdf = _make_default_diffuse()
        sig = _bsdf_signature(bsdf)
        if sig not in sig_to_id:
            bsdf_id = f"bsdf_{counter}"
            sig_to_id[sig] = bsdf_id
            counter += 1
            bsdf_map[bsdf_id] = bsdf
    return bsdf_map


def _sub_element(parent, tag, attrib=None, **extra):
    attrib = attrib or {}
    attrib.update(extra)
    return ET.SubElement(parent, tag, attrib)


def _emit_property(parent, tag, name, value):
    if isinstance(value, list):
        _sub_element(parent, tag, name=name, value=_vec3_str(value))
    else:
        _sub_element(parent, tag, name=name, value=str(value))


def _emit_transform(parent, ops: list[TransformOp]):
    if not ops:
        return
    t = _sub_element(parent, "transform", name="toWorld")
    for op in ops:
        if op.op_type == "lookat":
            _sub_element(t, "lookAt",
                         origin=_vec3_str(op.origin),
                         target=_vec3_str(op.target),
                         up=_vec3_str(op.up))
        elif op.op_type == "scale":
            _sub_element(t, "scale",
                         x=str(op.values[0]), y=str(op.values[1]), z=str(op.values[2]))
        elif op.op_type == "translate":
            _sub_element(t, "translate", value=_vec3_str(op.values))
        elif op.op_type == "rotate":
            axis = op.axis if op.axis else [0, 0, 1]
            _sub_element(t, "rotate",
                         x=str(axis[0]), y=str(axis[1]), z=str(axis[2]),
                         angle=str(op.angle))
        elif op.op_type == "matrix":
            _sub_element(t, "matrix", value=_vec3_str(op.values))


def _emit_bsdf_definition(parent, bsdf_id: str, bsdf):
    mapping = MITSUBA_BSDF_MAP.get(bsdf.type)
    if mapping is None:
        mapping = MITSUBA_BSDF_MAP["diffuse"]
    mitsuba_type, param_map = mapping
    el = _sub_element(parent, "bsdf", type=mitsuba_type, id=bsdf_id)
    for mitsuba_param, (nori_name, default_val) in param_map.items():
        if nori_name and nori_name in bsdf.params:
            val = bsdf.params[nori_name]
        else:
            val = default_val
        if isinstance(val, list):
            _emit_property(el, "rgb", mitsuba_param, val)
        elif isinstance(val, float):
            _emit_property(el, "float", mitsuba_param, val)
        elif isinstance(val, str):
            _emit_property(el, "string", mitsuba_param, val)


def _write_xml_pretty(root, filepath):
    raw = ET.tostring(root, encoding="unicode")
    dom = minidom.parseString(raw)
    pretty = dom.toprettyxml(indent="\t")
    lines = pretty.split("\n")
    result_lines = []
    for i, line in enumerate(lines):
        stripped = line.rstrip()
        if stripped == "":
            if result_lines and result_lines[-1] == "":
                continue
        result_lines.append(stripped)
    with open(filepath, "w") as f:
        f.write("\n".join(result_lines) + "\n")


def write_mitsuba(scene: Scene, output_path: str):
    root = ET.Element("scene", version="0.6.0")

    # Integrator
    mts_int = MITSUBA_INTEGRATOR_MAP.get(scene.integrator, "path")
    ET.SubElement(root, "integrator", type=mts_int)

    # Sensor (camera)
    cam = scene.camera
    sensor = ET.SubElement(root, "sensor", type="perspective")
    _emit_property(sensor, "float", "fov", cam.fov)
    _emit_property(sensor, "float", "nearClip", 1e-4)
    _emit_property(sensor, "float", "farClip", 1e4)

    # Camera transform (scale + lookAt merged)
    cam_ops = cam.transform
    if not cam_ops:
        cam_ops = [
            TransformOp(op_type="lookat", origin=[0, 0, 10],
                         target=[0, 0, 0], up=[0, 1, 0])
        ]
    _emit_transform(sensor, cam_ops)

    # Sampler inside sensor
    mts_sampler = MITSUBA_SAMPLER_MAP.get(scene.sampler.type, "independent")
    sampler = _sub_element(sensor, "sampler", type=mts_sampler)
    _emit_property(sampler, "integer", "sampleCount", scene.sampler.sample_count)

    # Film inside sensor
    film = _sub_element(sensor, "film", type="hdrfilm")
    _emit_property(film, "integer", "width", cam.width)
    _emit_property(film, "integer", "height", cam.height)
    ET.SubElement(film, "rfilter", type="gaussian")

    # BSDF definitions
    bsdf_map = _assign_bsdf_ids(scene)
    sig_to_id = {}
    for bsdf_id, bsdf in bsdf_map.items():
        sig = _bsdf_signature(bsdf)
        sig_to_id[sig] = bsdf_id
        _emit_bsdf_definition(root, bsdf_id, bsdf)

    # Standalone emitters (point lights)
    for emitter in scene.emitters:
        if emitter.type == "point":
            el = ET.SubElement(root, "emitter", type="point")
            pos = emitter.params.get("position", [0, 0, 0])
            power = emitter.params.get("power", [1, 1, 1])
            t = _sub_element(el, "transform", name="toWorld")
            _sub_element(t, "translate",
                         x=str(pos[0]), y=str(pos[1]), z=str(pos[2]))
            _emit_property(el, "rgb", "intensity", power)

    # Shapes
    for mesh in scene.meshes:
        shape = _sub_element(root, "shape", type="obj")

        # Absolute OBJ path (most reliable for cross-directory references)
        obj_path = mesh.obj_filename
        obj_abs = os.path.abspath(obj_path)
        _emit_property(shape, "string", "filename", obj_abs)

        # Mesh transform
        obj_ops = [op for op in mesh.transform if op.op_type != "lookat"]
        _emit_transform(shape, obj_ops)

        # BSDF reference
        bsdf = mesh.bsdf
        if bsdf is None:
            bsdf = _make_black_diffuse() if mesh.emitter else _make_default_diffuse()
        sig = _bsdf_signature(bsdf)
        if sig not in sig_to_id:
            new_id = f"bsdf_{len(bsdf_map)}"
            bsdf_map[new_id] = bsdf
            sig_to_id[sig] = new_id
            _emit_bsdf_definition(root, new_id, bsdf)
        bsdf_id = sig_to_id[sig]
        _sub_element(shape, "ref", id=bsdf_id)

        # Emitter (area light)
        if mesh.emitter and mesh.emitter.type == "area":
            radiance = mesh.emitter.params.get("radiance", [1, 1, 1])
            emitter_el = _sub_element(shape, "emitter", type="area")
            _emit_property(emitter_el, "rgb", "radiance", radiance)

    _write_xml_pretty(root, output_path)
