C: int
H: int
S: int
def f(L):
   global C, H, S

   C = L // 36
   L = L %  36

   H = (L >= 18) * 1
   S = L - H * 18 + 1

f(200)

print(C, H, S)