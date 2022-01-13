float dot2( in vec2 v ) { return dot(v,v); }
float dot2( in vec3 v ) { return dot(v,v); }
float ndot( in vec2 a, in vec2 b ) { return a.x*b.x - a.y*b.y; }

float sdfCircle(vec2 uv, float r,vec2 offset) {
    float x = uv.x - offset.x;
    float y = uv.y - offset.y;
    
    float d = length(vec2(x, y)) - r;
    return d;
}

vec2 rotate(vec2 uv, float th) {
  return mat2(cos(th), sin(th), -sin(th), cos(th)) * uv;
}

float sdfSquare(vec2 uv, float size,vec2 offset) {
    float x = uv.x - offset.x;
    float y = uv.y - offset.y;
    vec2 rotated = rotate(vec2(x,y),iTime); //绕着自身旋转
    float d = max(abs(rotated.x),abs(rotated.y)) - size;
    return d;
}

vec3 getBgColor(vec2 uv)
{
    //it assumps the uv is in [-0.5,0.5]
    uv += 0.5;
    vec3 gradientStartColor = vec3(1,0,1);
    vec3 gradientEndColor = vec3(0,1,1);
    return mix(gradientStartColor,gradientEndColor,uv.y);
}

float smin(float a, float b, float k) {
    //http://www.viniciusgraciano.com/blog/smin/
  float h = clamp(0.5+0.5*(b-a)/k, 0.0, 1.0);
  return mix(b, a, h) - k*h*(1.0-h);
}

float smax(float a, float b, float k) {
  return -smin(-a, -b, k);
}

float union_sdf(float d1,float d2)
{
    return min(d1,d2);
}

float smooth_union_sdf(float d1,float d2)
{
    return smin(d1,d2,0.01);
}

float intersection_sdf(float d1,float d2)
{
    //在某个图形外的为正，该部分不会显示
    //所以是交集
    return max(d1,d2);
}

float smooth_intersection_sdf(float d1,float d2)
{
    return smax(d1,d2,0.01);
}

float substraction_sdf(float d1,float d2)
{
    //取反以后求交，相当于布尔求差
    return max(-d1,d2);
}

float xor_sdf(float d1,float d2)
{
    //求不相交的地方
    return intersection_sdf(union_sdf(d1,d2),-max(d1,d2));
}

float opSymX(vec2 p, float r)
{
    //create a duplicate shape along x-asix with 0.2 offset
  p.x = abs(p.x);
  return sdfCircle(p, r, vec2(0.2, 0));
}

float opSymY(vec2 p, float r)
{
    //create a duplicate shape along x-asix with 0.2 offset
  p.y = abs(p.y);
  return sdfCircle(p, r, vec2(0, 0.2));
}

float opSymXY(vec2 p, float r)
{
    //create a duplicate shape along x-asix with 0.2 offset
    p = abs(p);
  return sdfCircle(p, r, vec2(0.2));
}

float opRep(vec2 p, float r, vec2 c)
{
  vec2 q = mod(p+0.5*c,c)-0.5*c;
  return sdfCircle(q, r, vec2(0));
}

float opRepLim(vec2 p, float r, float c, vec2 l)
{
  vec2 q = p-c*clamp(round(p/c),-l,l);
  return sdfCircle(q, r, vec2(0));
}


vec3 drawScene(vec2 uv)
{
    vec3 col = getBgColor(uv);
    float d_circle = sdfCircle(uv,0.1,vec2(0.0));
    // float d_square = sdfSquare(uv,0.07,vec2(0.1,0));
    // float res = smooth_intersection_sdf(d_circle,d_square);
    // float res = opSymX(uv,0.1);
    // res = union_sdf(res,opSymY(uv,0.1));
    float res = opRepLim(uv, 0.05, 0.2,vec2(2,2));
    // res = step(0.,res);
    //res为0的时候代表硬边缘，在0附近插值可以抗锯齿
    res = smoothstep(-0.001,0.001,res);

    col = mix(vec3(0,0,1),col,res);
    return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  vec2 uv = fragCoord/iResolution.xy; // <0,1>
  uv -= 0.5;
  uv.x *= iResolution.x / iResolution.y;
  //u不再是[0,1],因为w不等于h，因此如果uv属于相同区间，那么图形就会有拉伸
  //放大u的话，可以使得[0,1]在wh方向上有同样的像素数量

  vec2 offset = vec2(sin(iTime) * 0.2,cos(iTime) * 0.2);
//   vec3 col = sdfCircle(uv, .1,offset); // Call this function on each pixel to check if the coordinate lies inside or outside of the circle
//   vec3 col = sdfSquare(uv,0.1,offset);
  // Output to screen
  fragColor = vec4(drawScene(uv),1.0);
}
