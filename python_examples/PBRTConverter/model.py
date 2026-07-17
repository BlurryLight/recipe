from dataclasses import dataclass, field
from typing import Optional


@dataclass
class LookAt:
    origin: tuple[float, float, float]
    target: tuple[float, float, float]
    up: tuple[float, float, float] = (0, 1, 0)


@dataclass
class TransformOp:
    op_type: str  # "scale", "translate", "rotate", "lookat", "matrix"
    values: list[float] = field(default_factory=list)
    axis: Optional[list[float]] = None
    angle: float = 0.0
    origin: Optional[list[float]] = None
    target: Optional[list[float]] = None
    up: Optional[list[float]] = None


@dataclass
class BSDF:
    type: str  # "diffuse", "mirror", "dielectric", "microfacet", "glass"
    params: dict = field(default_factory=dict)


@dataclass
class Emitter:
    type: str  # "area", "point"
    params: dict = field(default_factory=dict)


@dataclass
class Mesh:
    obj_filename: str
    bsdf: Optional[BSDF] = None
    emitter: Optional[Emitter] = None
    transform: list[TransformOp] = field(default_factory=list)


@dataclass
class Camera:
    type: str = "perspective"
    fov: float = 30.0
    width: int = 1280
    height: int = 720
    transform: list[TransformOp] = field(default_factory=list)


@dataclass
class Sampler:
    type: str = "independent"
    sample_count: int = 16


@dataclass
class Scene:
    integrator: str = "path"
    camera: Camera = field(default_factory=Camera)
    sampler: Sampler = field(default_factory=Sampler)
    meshes: list[Mesh] = field(default_factory=list)
    emitters: list[Emitter] = field(default_factory=list)


# BSDF mapping: Nori type -> (pbrt_material_name, param_mapping)
BSDF_MAP = {
    "diffuse": {
        "material": "matte",
        "params": {
            "Kd": ("color", "albedo", [0.5, 0.5, 0.5]),
        },
    },
    "mirror": {
        "material": "mirror",
        "params": {
            "Kr": ("rgb", None, [1.0, 1.0, 1.0]),
        },
    },
    "dielectric": {
        "material": "glass",
        "params": {
            "Kr": ("rgb", "specularReflectance", [1.0, 1.0, 1.0]),
            "Kt": ("rgb", "specularTransmittance", [1.0, 1.0, 1.0]),
            "eta": ("float", "intIOR", 1.5046),
        },
    },
    "glass": {
        "material": "glass",
        "params": {
            "Kr": ("rgb", "specularReflectance", [1.0, 1.0, 1.0]),
            "Kt": ("rgb", "specularTransmittance", [1.0, 1.0, 1.0]),
            "eta": ("float", "intIOR", 1.5046),
            "roughness": ("float", "alpha", 0.1),
        },
    },
    "microfacet": {
        "material": "plastic",
        "params": {
            "Kd": ("color", "kd", [0.5, 0.5, 0.5]),
            "Ks": ("color", None, [1.0, 1.0, 1.0]),
            "roughness": ("float", "alpha", 0.1),
        },
    },
}

# Integrator mapping: Nori type -> pbrt integrator name
INTEGRATOR_MAP = {
    "path_mis": "path",
    "path_mats": "path",
    "path_ems": "path",
    "path_nee": "path",
    "direct_mis": "directlighting",
    "direct_mats": "directlighting",
    "direct_ems": "directlighting",
    "whitted": "whitted",
    "ao": "ambientocclusion",
    "normals": None,
    "simple": None,
}

# Sampler mapping
SAMPLER_MAP = {
    "independent": "random",
    "halton": "halton",
}
