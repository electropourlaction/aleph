#include "net_protected.h"
#include "op_mul.h"

//-------------------------------------------------
//----- static function declaration
static void op_mul_inc_input(op_mul_t* mul, const s16 idx, const io_t inc);
static void op_mul_in_a(op_mul_t* mul, const io_t* v);
static void op_mul_in_b(op_mul_t* mul, const io_t* v);
static void op_mul_in_btrig(op_mul_t* mul, const io_t* v);

//-------------------------------------------------
//----- static vars
static op_in_func_t op_mul_in_func[3] = {
  (op_in_func_t)&op_mul_in_a,
  (op_in_func_t)&op_mul_in_b,
  (op_in_func_t)&op_mul_in_btrig
};

static const char* op_mul_instring  = "A       B       B_TRIG  ";
static const char* op_mul_outstring = "VAL     ";
static const char* op_mul_opstring  = "MUL";

//-------------------------------------------------
//----- external function definitions
void op_mul_init(op_mul_t* mul) {
  mul->super.numInputs = 3;
  mul->super.numOutputs = 1;
  mul->outs[0] = -1;
  mul->super.inc_func = (op_inc_func)op_mul_inc_input;
  mul->super.in_func = op_mul_in_func;
  mul->super.in_val = mul->in_val;
  mul->super.out = mul->outs;
  mul->super.opString = op_mul_opstring;
  mul->super.inString = op_mul_instring;
  mul->super.outString = op_mul_outstring;
  mul->super.type = eOpMul;  
  mul->super.status = eUserOp;  
 
  mul->in_val[0] = &(mul->a);
  mul->in_val[1] = &(mul->b);
  mul->in_val[2] = &(mul->btrig);
}

//-------------------------------------------------
//----- static function definitions
// set operand A
static void op_mul_in_a(op_mul_t* mul, const io_t* v) {
  // printf("mul at %d received A %d\n", (int)mul, (int)*v);
  mul->a = *v;
  mul->val = OP_MUL(mul->a, mul->b);
  net_activate(mul->outs[0], mul->val);
}

// set operand B
static void op_mul_in_b(op_mul_t* mul, const io_t* v) {
  //printf("mul at %d received B %d\n", (int)mul, (int)*v);
  mul->b = *v;
  mul->val = OP_MUL(mul->a, mul->b);
  if(mul->btrig) {
    net_activate(mul->outs[0], mul->val);
  }
}

// set b-trigger mode
static void op_mul_in_btrig(op_mul_t* mul, const io_t* v) {
  //printf("mul at %d received BTRIG %d\n", (int)mul, (int)*v);
  if(*v > 0) { mul->btrig = OP_ONE; } { mul->btrig = 0; }
}

//===== UI input
static void op_mul_inc_input(op_mul_t* mul, const s16 idx, const io_t inc) {
  io_t val;
  switch(idx) {
  case 0:  // a
    val = OP_ADD(mul->a, inc);
    op_mul_in_a(mul, &val);
    break; 
  case 1:  // b
    val = OP_ADD(mul->b, inc);
    op_mul_in_b(mul, &val);
    break;
  case 2:  // trig
    op_mul_in_btrig(mul, &inc);
    break;
  }
}
