precision mediump float;
const int MAX_MARCHING_STEPS = 255;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float PRECISION = 0.001;

vec3 getBgColor(vec2 uv)
{
  vec3 startColor = vec3(1,0,1);
  vec3 endColor = vec3(0,1,1);
  return mix(startColor,endColor,uv.y);
}

vec4 sdSphere(vec3 p, float r ,vec3 offset,vec3 color)
{
  return vec4(length(p - offset) - r,color);
}

vec4 sdFloor(vec3 p,float offset,vec3 col)
{
  return vec4(p.y - offset,col);
}

vec4 minWithColor(vec4 obj1,vec4 obj2)
{
  return obj2.x < obj1.x ? obj2 : obj1;
}

vec4 sdScene(vec3 p)
{
  vec4 res1 = sdSphere(p,1.,vec3(0,-1,-2),vec3(0,.8,.8));
  vec4 res2 = sdSphere(p,1.,vec3(-1,1,-2),vec3(1,0.58,.29));
  vec3 floorColor = vec3(1. + 0.7*mod(floor(2. * p.x) + floor(p.z), 2.0));
  vec4 res3 = sdFloor(p,-2.,floorColor);
  vec4 co = minWithColor(res1,res2);
  vec4 res = minWithColor(co,res3);
  return res;
}

vec4 rayMarch(vec3 ro, vec3 rd, float start, float end) {
  float depth = start;
  vec4 co;
  for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
    vec3 p = ro + depth * rd;
    co = sdScene(p);
    float d = co.x;
    depth += co.x;
    if (d < PRECISION || depth > end) break;
  }

  return vec4(depth,co.yzw);
}

vec3 calcNormal(vec3 p) {
    vec2 e = vec2(1.0, -1.0) * 0.0005; // epsilon
    float r = 1.; // radius of sphere
    return normalize(
      e.xyy * sdScene(p + e.xyy).x +
      e.yyx * sdScene(p + e.yyx).x +
      e.yxy * sdScene(p + e.yxy).x +
      e.xxx * sdScene(p + e.xxx).x);
}

// vec3 calcNormal(vec3 p) {
//   float e = 0.0005; // epsilon
//   float r = 1.; // radius of sphere
//   return normalize(vec3(
//     (sdSphere(vec3(p.x + e, p.y, p.z), r) - sdSphere(vec3(p.x - e, p.y, p.z), r)),
//     sdSphere(vec3(p.x, p.y + e, p.z), r) - sdSphere(vec3(p.x, p.y - e, p.z), r),
//     sdSphere(vec3(p.x, p.y, p.z  + e), r) - sdSphere(vec3(p.x, p.y, p.z - e), r)
//   ));
// }
// 

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;

  vec3 col = vec3(0);
  vec3 ro = vec3(0, 0, 3); // ray origin that represents camera position
  vec3 rd = normalize(vec3(uv, -1)); // ray direction

  vec4 co = rayMarch(ro, rd, MIN_DIST, MAX_DIST); // distance to sphere
  float d = co.x;

  if (d > MAX_DIST) {
    // col = vec3(0.6); // ray didn't hit anything
    col = getBgColor(uv + 0.5);
  } else {
    vec3 p = ro + rd * d; // point on sphere we discovered from ray marching
    vec3 normal = calcNormal(p);
    vec3 lightPosition = vec3(2, 2, 4. * cos(iTime));
    vec3 lightDirection = normalize(lightPosition - p);

    // Calculate diffuse reflection by taking the dot product of 
    // the normal and the light direction.
    float dif = clamp(dot(normal, lightDirection), 0., 1.);
    const vec3 ambient = vec3(0.2);

    // col = clamp((ambient + vec3(dif)) * co.yzw),0.,1.);
    col = clamp((ambient + dif) * (co.yzw),0.,1.);
  }

  // Output to screen
  fragColor = vec4(col, 1.0);
}
