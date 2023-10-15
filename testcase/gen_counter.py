sz = 4
M  = 2 * sz
Ands = []
carries = []
results = []

def Lit(a):
    if(a > 0):
        return a*2
    else:
        return (-a)*2+1


def And(r, a, b):
    global And
    Ands.append([Lit(r), Lit(a), Lit(b)])

def Xor(a, b):
    global M
    And(M+1, a, -b)
    And(M+2, -a, b)
    And(M+3, -M-1, -M-2)
    M += 3
    return -M

def res2(a, b):
    global results
    res = Xor(a, b)
    results.append(res)

def res3(a, b, c):
    global results
    res1 = Xor(a, b)
    res2 = Xor(res1, c)
    results.append(res2)

def carry2(a, b):
    global M,carries
    And(M+1, a, b)
    M += 1
    carries.append(M)

def carry3(a, b, c):
    global M, carries
    And(M+1, a, b)
    And(M+2, a, c)
    And(M+3, b, c)
    And(M+4, -M-1, -M-2)
    And(M+5, M+4, -M-3)
    M += 5
    carries.append(-M)

res2(1, sz+1)
carry2(1,sz+1)
for i in range(2, sz):
    last_c = carries[len(carries)-1]
    res3(i, sz+i, last_c)
    carry3(i, sz+i, last_c)
last_c = carries[len(carries)-1]
res3(sz, sz+sz, last_c)

aag_str = "aag "+str(M)+" "+str(sz)+" "+str(sz)+" 0 "+str(len(Ands))+" 1 "+str(sz-1)+"\n"
for i in range(sz):
    aag_str += (str(2*(i+1))+"\n")
for i in range(sz):
    aag_str += (str(2*(sz + i+1)) + " " + str(Lit(results[i])) + " 0\n")
aag_str += str(sz*2*2) + "\n"
for i in range(sz-1):
    aag_str += str(2*(sz - i)+1) + "\n"
for a in Ands:
    aag_str += str(a[0])+" "+str(a[1])+" "+str(a[2])+"\n"
aag_str+="c\n"
aag_str+="inputs: ".ljust(15)
for i in range(sz):
    aag_str+=str(sz-i).rjust(5)
aag_str+="\n"
aag_str+="latches:".ljust(15)
for i in range(sz):
    aag_str+=str(sz+sz-i).rjust(5)
aag_str+="\n"
aag_str+="+    --------------------------------------\n"
aag_str+="carries: ".ljust(15)
for i in range(sz-1):
    aag_str+=str(carries[sz-2-i]).rjust(5)
aag_str+="\n"
aag_str+="results: ".ljust(15)
for i in range(sz):
    aag_str+=str(results[sz-1-i]).rjust(5)
aag_str+="\n"

with open("./test_counter_4.aag",'w') as f:
    f.write(aag_str)