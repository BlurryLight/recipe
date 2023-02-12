import testLib

print(testLib.add(1, 2))

vec0 = testLib.Vec3f(0, 1, 2)
vec1 = testLib.Vec3f(3, 4, 5)
vec2 = vec0 + vec1
vec2.Display()
# 从 std::stirng到 python string
s: str = vec2.Text()
print(s.upper())

print("Vec2 {} {} {}".format(vec2.x, vec2.y, vec2.z))

circle = testLib.Circle(2)
print(circle.area())
print(circle.nShapes)
circle = None

print(testLib.cvar.Shape_nShapes)

# vector test
arr = testLib.IntVector([1, 2, 3, 4, 5, 6])
print(testLib.average(arr))
