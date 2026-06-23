import struct, sys

ABI={'zero':0,'ra':1,'sp':2,'gp':3,'tp':4,'t0':5,'t1':6,'t2':7,'s0':8,'fp':8,'s1':9,
'a0':10,'a1':11,'a2':12,'a3':13,'a4':14,'a5':15,'a6':16,'a7':17,'s2':18,'s3':19,
's4':20,'s5':21,'s6':22,'s7':23,'s8':24,'s9':25,'s10':26,'s11':27,'t3':28,'t4':29,'t5':30,'t6':31}
def reg(s):
    s=s.strip()
    if s in ABI: return ABI[s]
    if s.startswith('x'): return int(s[1:])
    raise ValueError("reg? "+s)

def R(f7,rs2,rs1,f3,rd,op): return (f7<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op
def Itype(im,rs1,f3,rd,op): return ((im&0xfff)<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op
def S(im,rs2,rs1,f3,op):
    return (((im>>5)&0x7f)<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|((im&0x1f)<<7)|op
def Btype(im,rs2,rs1,f3,op):
    return (((im>>12)&1)<<31)|(((im>>5)&0x3f)<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(((im>>1)&0xf)<<8)|(((im>>11)&1)<<7)|op
def U(im,rd,op): return ((im&0xfffff)<<12)|(rd<<7)|op
def Jtype(im,rd,op):
    return (((im>>20)&1)<<31)|(((im>>1)&0x3ff)<<21)|(((im>>11)&1)<<20)|(((im>>12)&0xff)<<12)|(rd<<7)|op

def expand(op, args):
    if op=='li':
        rd, imm = args[0], int(args[1],0)
        if -2048 <= imm <= 2047:
            return [('addi',[rd,'zero',str(imm)])]
        hi = (imm + 0x800) >> 12 & 0xfffff
        lo = imm - (hi<<12)
        lo = ((lo + 0x800) & 0xfff) - 0x800
        return [('lui',[rd,str(hi)]), ('addi',[rd,rd,str(lo)])]
    if op=='mv':   return [('addi',[args[0],args[1],'0'])]
    if op=='nop':  return [('addi',['zero','zero','0'])]
    if op=='j':    return [('jal',['zero',args[0]])]
    if op=='call': return [('jal',['ra',args[0]])]
    if op=='ret':  return [('jalr',['zero','0(ra)'])]
    if op=='neg':  return [('sub',[args[0],'zero',args[1]])]
    if op=='not':  return [('xori',[args[0],args[1],'-1'])]
    if op=='beqz': return [('beq',[args[0],'zero',args[1]])]
    if op=='bnez': return [('bne',[args[0],'zero',args[1]])]
    if op=='bgt':  return [('blt',[args[1],args[0],args[2]])]
    if op=='ble':  return [('bge',[args[1],args[0],args[2]])]
    return [(op,args)]

def encode(pc, op, a, labels):
    def lbl(x, rel):
        x=x.strip()
        if x=='.': return 0 if rel else pc
        if x in labels: return labels[x]-(pc if rel else 0)
        return int(x,0)
    def memparse(x):
        o,r=x.split('(');
        o=o.strip()
        return (labels[o] if o in labels else int(o,0)), reg(r.rstrip(')'))
    if op=='addi': return Itype(int(a[2],0),reg(a[1]),0,reg(a[0]),0x13)
    if op=='slti': return Itype(int(a[2],0),reg(a[1]),2,reg(a[0]),0x13)
    if op=='sltiu':return Itype(int(a[2],0),reg(a[1]),3,reg(a[0]),0x13)
    if op=='xori': return Itype(int(a[2],0),reg(a[1]),4,reg(a[0]),0x13)
    if op=='ori':  return Itype(int(a[2],0),reg(a[1]),6,reg(a[0]),0x13)
    if op=='andi': return Itype(int(a[2],0),reg(a[1]),7,reg(a[0]),0x13)
    if op=='slli': return Itype(int(a[2],0),reg(a[1]),1,reg(a[0]),0x13)
    if op=='srli': return Itype(int(a[2],0),reg(a[1]),5,reg(a[0]),0x13)
    if op=='srai': return Itype(0x400|int(a[2],0),reg(a[1]),5,reg(a[0]),0x13)
    if op=='add': return R(0,reg(a[2]),reg(a[1]),0,reg(a[0]),0x33)
    if op=='sub': return R(0x20,reg(a[2]),reg(a[1]),0,reg(a[0]),0x33)
    if op=='sll': return R(0,reg(a[2]),reg(a[1]),1,reg(a[0]),0x33)
    if op=='slt': return R(0,reg(a[2]),reg(a[1]),2,reg(a[0]),0x33)
    if op=='sltu':return R(0,reg(a[2]),reg(a[1]),3,reg(a[0]),0x33)
    if op=='xor': return R(0,reg(a[2]),reg(a[1]),4,reg(a[0]),0x33)
    if op=='srl': return R(0,reg(a[2]),reg(a[1]),5,reg(a[0]),0x33)
    if op=='sra': return R(0x20,reg(a[2]),reg(a[1]),5,reg(a[0]),0x33)
    if op=='or':  return R(0,reg(a[2]),reg(a[1]),6,reg(a[0]),0x33)
    if op=='and': return R(0,reg(a[2]),reg(a[1]),7,reg(a[0]),0x33)
    if op=='lui': return U(int(a[1],0),reg(a[0]),0x37)
    if op=='auipc':return U(int(a[1],0),reg(a[0]),0x17)
    if op in('lb','lh','lw','lbu','lhu'):
        o,r=memparse(a[1]); f3={'lb':0,'lh':1,'lw':2,'lbu':4,'lhu':5}[op]
        return Itype(o,r,f3,reg(a[0]),0x03)
    if op in('sb','sh','sw'):
        o,r=memparse(a[1]); f3={'sb':0,'sh':1,'sw':2}[op]
        return S(o,reg(a[0]),r,f3,0x23)
    if op in('beq','bne','blt','bge','bltu','bgeu'):
        f3={'beq':0,'bne':1,'blt':4,'bge':5,'bltu':6,'bgeu':7}[op]
        return Btype(lbl(a[2],True),reg(a[1]),reg(a[0]),f3,0x63)
    if op=='jal': return Jtype(lbl(a[1],True),reg(a[0]),0x6f)
    if op=='jalr':
        o,r=memparse(a[1]); return Itype(o,r,0,reg(a[0]),0x67)
    if op=='ecall': return 0x73
    raise ValueError("op desconocido: "+op)

def assemble(text):
    raw_lines=[]
    for ln in text.split('\n'):
        ln=ln.split('#')[0].strip()
        if not ln: continue
        if ln.startswith('.'):
            if ln.startswith('.global') or ln.startswith('.globl') or ln.startswith('.text') or ln.startswith('.data'):
                continue
        raw_lines.append(ln)

    items=[]
    labels={}
    pc=0
    for ln in raw_lines:
        while ':' in ln:
            idx=ln.index(':')
            lbl=ln[:idx].strip()
            labels[lbl]=pc
            ln=ln[idx+1:].strip()
            if not ln: break
        if not ln: continue
        parts=ln.replace(',',' ').split()
        op=parts[0]; args=parts[1:]
        for (rop,rargs) in expand(op,args):
            items.append((rop,rargs)); pc+=4

    items=[]; labels={}; pc=0
    for ln in raw_lines:
        while ':' in ln:
            idx=ln.index(':'); lbl=ln[:idx].strip(); labels[lbl]=pc
            ln=ln[idx+1:].strip()
            if not ln: break
        if not ln: continue
        parts=ln.replace(',',' ').split()
        op=parts[0]; args=parts[1:]
        for (rop,rargs) in expand(op,args):
            items.append((pc,rop,rargs)); pc+=4

    out=b''
    for (pc,op,args) in items:
        w=encode(pc,op,args,labels)
        out+=struct.pack('<I', w & 0xffffffff)
    return out, labels

if __name__=='__main__':
    txt=open(sys.argv[1]).read()
    data,labels=assemble(txt)
    open(sys.argv[2],'wb').write(data)
    print(f"{len(data)} bytes ({len(data)//4} instrucciones) -> {sys.argv[2]}")
