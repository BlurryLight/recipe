# // see https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
import numpy as np

# Using the above nested radical formula for g=phi_d 
# or you could just hard-code it. 
# phi(1) = 1.6180339887498948482 
# phi(2) = 1.32471795724474602596 

# 广义黄金分割phi(d)，满足 x^(d+1) = x + 1 的正实数解
# x是一个[1,2]之间的数值，初始值可以任取[1,2]之间的任意数字，不影响收敛
def phi(d): 
  assert(d>0)
  x=2.0000 
  for _ in range(10): 
    x = pow(1+x,1/(d+1)) 
  return x

# Number of dimensions. 
d=2 

# number of required points 
n=50 

g = phi(d) 
alpha = np.zeros(d) 
for j in range(d): 
  alpha[j] = pow(1/g,j+1) % 1 
z = np.zeros((n, d)) 

# This number can be any real number. 
# Common default setting is typically seed=0
# But seed = 0.5 might be marginally better. 
seed = 0.5
for i in range(n): 
  z[i] = (seed + alpha*(i+1)) %1 
print(z)