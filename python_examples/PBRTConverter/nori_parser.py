import xml.etree.ElementTree as ET
import os
from .model import (
    Scene, Camera, Sampler, Mesh, BSDF, Emitter, TransformOp, LookAt,
    INTEGRATOR_MAP, SAMPLER_MAP,
)


def _parse_float_list(text: str) -> list[float]:
    return [float(x.strip()) for x in text.replace(",", " ").split()]


def _parse_transform(element) -> list[TransformOp]:
    ops = []
    for child in element:
        tag = child.tag
        if tag == "scale":
            vals = _parse_float_list(child.attrib.get("value", "1,1,1"))
            ops.append(TransformOp(op_type="scale", values=vals))
        elif tag == "translate":
            vals = _parse_float_list(child.attrib.get("value", "0,0,0"))
            ops.append(TransformOp(op_type="translate", values=vals))
        elif tag == "rotate":
            angle = float(child.attrib.get("angle", 0))
            axis = _parse_float_list(child.attrib.get("axis", "0,0,1") if "axis" in child.attrib else child.attrib.get("value", "0,0,1"))
            ops.append(TransformOp(op_type="rotate", angle=angle, axis=axis))
        elif tag == "lookat":
            origin = _parse_float_list(child.attrib.get("origin", "0,0,0"))
            target = _parse_float_list(child.attrib.get("target", "0,0,-1"))
            up = _parse_float_list(child.attrib.get("up", "0,1,0"))
            ops.append(TransformOp(op_type="lookat",
                                   origin=origin, target=target, up=up))
        elif tag == "matrix":
            vals = _parse_float_list(child.attrib.get("value", ""))
            ops.append(TransformOp(op_type="matrix", values=vals))
    return ops


def _parse_bsdf(element) -> BSDF:
    bsdf_type = element.attrib.get("type", "diffuse")
    params = {}
    for child in element:
        tag = child.tag
        name = child.attrib.get("name", "")
        value = child.attrib.get("value", "")
        if tag == "color":
            params[name] = _parse_float_list(value)
        elif tag == "float":
            params[name] = float(value)
        elif tag == "integer":
            params[name] = int(value)
        elif tag == "boolean":
            params[name] = value.lower() == "true"
        elif tag == "string":
            params[name] = value
    return BSDF(type=bsdf_type, params=params)


def _parse_emitter(element) -> Emitter:
    emitter_type = element.attrib.get("type", "area")
    params = {}
    for child in element:
        tag = child.tag
        name = child.attrib.get("name", "")
        value = child.attrib.get("value", "")
        if tag == "color":
            params[name] = _parse_float_list(value)
        elif tag == "float":
            params[name] = float(value)
        elif tag == "integer":
            params[name] = int(value)
        elif tag == "point":
            params[name] = _parse_float_list(value)
        elif tag == "vector":
            params[name] = _parse_float_list(value)
    return Emitter(type=emitter_type, params=params)


def parse_nori_scene(filepath: str) -> Scene:
    tree = ET.parse(filepath)
    root = tree.getroot()
    base_dir = os.path.dirname(os.path.abspath(filepath))

    scene = Scene()

    for element in root:
        tag = element.tag

        if tag == "integrator":
            nori_type = element.attrib.get("type", "path")
            scene.integrator = INTEGRATOR_MAP.get(nori_type, nori_type)

        elif tag == "camera":
            cam = Camera()
            cam.type = element.attrib.get("type", "perspective")
            for child in element:
                child_tag = child.tag
                if child_tag == "float":
                    name = child.attrib.get("name", "")
                    value = float(child.attrib.get("value", 0))
                    if name == "fov":
                        cam.fov = value
                elif child_tag == "integer":
                    name = child.attrib.get("name", "")
                    value = int(child.attrib.get("value", 0))
                    if name == "width":
                        cam.width = value
                    elif name == "height":
                        cam.height = value
                elif child_tag == "transform":
                    cam.transform = _parse_transform(child)
            scene.camera = cam

        elif tag == "sampler":
            smp = Sampler()
            smp.type = element.attrib.get("type", "independent")
            smp.type = SAMPLER_MAP.get(smp.type, smp.type)
            for child in element:
                if child.tag == "integer" and child.attrib.get("name") == "sampleCount":
                    smp.sample_count = int(child.attrib.get("value", 16))
            scene.sampler = smp

        elif tag == "mesh":
            obj_filename = ""
            transform = []
            bsdf = None
            emitter = None

            for child in element:
                if child.tag == "string" and child.attrib.get("name") == "filename":
                    obj_filename = child.attrib.get("value", "")
                elif child.tag == "bsdf":
                    bsdf = _parse_bsdf(child)
                elif child.tag == "emitter":
                    emitter = _parse_emitter(child)
                elif child.tag == "transform":
                    transform = _parse_transform(child)

            if obj_filename:
                full_path = os.path.join(base_dir, obj_filename)
                scene.meshes.append(Mesh(
                    obj_filename=full_path,
                    bsdf=bsdf,
                    emitter=emitter,
                    transform=transform,
                ))

        elif tag == "emitter":
            scene.emitters.append(_parse_emitter(element))

    return scene
