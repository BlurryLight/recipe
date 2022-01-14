const int MAX_MARCHING_STEPS = 255;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float PRECISION = 0.001;
const float EPSILON = 0.0005;

float sdSphere(vec3 p, float r, vec3 offset)
{
  return length(p - offset) - r;
}

float sdFloor(vec3 p) {
  return p.y + 1.;
}

float scene(vec3 p) {
  float co = min(sdSphere(p, 1., vec3(0, 0, -2)), sdFloor(p));
  return co;
}

float rayMarch(vec3 ro, vec3 rd) {
  float depth = MIN_DIST;
  float d; // distance ray has travelled

  for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
    vec3 p = ro + depth * rd;
    d = scene(p);
    depth += d;
    if (d < PRECISION || depth > MAX_DIST) break;
  }
  
  d = depth;
  
  return d;
}

vec3 calcNormal(in vec3 p) {
    vec2 e = vec2(1, -1) * EPSILON;
    return normalize(
      e.xyy * scene(p + e.xyy) +
      e.yyx * scene(p + e.yyx) +
      e.yxy * scene(p + e.yxy) +
      e.xxx * scene(p + e.xxx));
}

float softShadow(vec3 ro, vec3 rd, float mint, float maxt,float k) {
  //sdf shadow
  //沿着光源方向步进，步进的距离越多，说明影子越淡
  //一是距离相交点越近，通常阴影越深颜色越暗。二是距离着色点越近(也就是距离光源也远)，
  //https://zhuanlan.zhihu.com/p/372506985 中间部分的解释
  float res = 1.0;

  float t = mint;
  for(int i = 0; i < 16; i++) //最多步进16步
  {
    float h = scene(ro + rd * t);
    t += clamp(h, 0.02, 0.10);
    res = min(res,k * h / t);
    if(h < 0.001 || t > maxt) break;
  }

  // for(int i = 0; i < 16; i++) {
  //   float h = scene(ro + rd * t);
  //     res = min(res, k*h/t);
  //     t += clamp(h, 0.02, 0.10);
  //     if(h < 0.001 || t > tmax) break;
  // }

  return clamp( res, 0.0, 1.0 );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
  vec3 backgroundColor = vec3(0);

  vec3 col = vec3(0);
  vec3 ro = vec3(0, 0, 3); // ray origin that represents camera position
  vec3 rd = normalize(vec3(uv, -1)); // ray direction

  float sd = rayMarch(ro, rd); // signed distance value to closest object

  if (sd > MAX_DIST) {
    col = backgroundColor; // ray didn't hit anything
  } else {
    vec3 p = ro + rd * sd; // point discovered from ray marching
    vec3 normal = calcNormal(p); // surface normal

    vec3 lightPosition = vec3(cos(iTime), 2, sin(iTime));
    vec3 lightDirection = normalize(lightPosition - p);

    float dif = clamp(dot(normal, lightDirection), 0., 1.); // diffuse reflection clamped between zero and one

  vec3 newRayOrigin = p + normal * PRECISION * 2.;
  //   float shadowRayLength = rayMarch(newRayOrigin, lightDirection);
  //   if (shadowRayLength < length(lightPosition - newRayOrigin)) dif *= 0.1;
  //   col = vec3(dif);
    float softShadow = clamp(softShadow(newRayOrigin, lightDirection, 0.02, 2.5,4.), 0.1, 1.0);
    col = vec3(dif)  * softShadow;

  }
  col = mix(col,vec3(1.0), 1.0 - exp(-0.002 * sd * sd * sd));//fog

  fragColor = vec4(pow(col,vec3(1.0/2.2)), 1.0);
}
