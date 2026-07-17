import os


class OBJMesh:
    def __init__(self):
        self.positions = []
        self.normals = []
        self.texcoords = []
        self.indices = []

    def is_empty(self):
        return len(self.positions) == 0 or len(self.indices) == 0


def read_obj(filepath: str) -> OBJMesh:
    mesh = OBJMesh()
    positions = []
    normals = []
    texcoords = []
    face_vertices = []

    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if not parts:
                continue

            if parts[0] == "v":
                positions.append((
                    float(parts[1]),
                    float(parts[2]),
                    float(parts[3]),
                ))
            elif parts[0] == "vn":
                normals.append((
                    float(parts[1]),
                    float(parts[2]),
                    float(parts[3]),
                ))
            elif parts[0] == "vt":
                texcoords.append((
                    float(parts[1]),
                    float(parts[2]),
                ))
            elif parts[0] == "f":
                verts = []
                for i in range(1, len(parts)):
                    indices = parts[i].split("/")
                    vi = int(indices[0]) - 1
                    ti = int(indices[1]) - 1 if len(indices) > 1 and indices[1] else -1
                    ni = int(indices[2]) - 1 if len(indices) > 2 and indices[2] else -1
                    verts.append((vi, ti, ni))
                face_vertices.append(verts)

    if not face_vertices:
        return mesh

    vertex_map = {}
    idx_counter = 0

    for face in face_vertices:
        for vi, ti, ni in face:
            if vi < 0 or vi >= len(positions):
                continue
            key = (vi, ti, ni)
            if key not in vertex_map:
                vertex_map[key] = idx_counter
                idx_counter += 1
                mesh.positions.append(positions[vi])
                if ni >= 0 and ni < len(normals):
                    mesh.normals.append(normals[ni])
                if ti >= 0 and ti < len(texcoords):
                    mesh.texcoords.append(texcoords[ti])

    for face in face_vertices:
        if len(face) < 3:
            continue
        for k in range(1, len(face) - 1):
            v0_key = face[0]
            v1_key = face[k]
            v2_key = face[k + 1]
            mesh.indices.append(vertex_map[v0_key])
            mesh.indices.append(vertex_map[v1_key])
            mesh.indices.append(vertex_map[v2_key])

    return mesh
