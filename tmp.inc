
#define OPCODE(opc,  strname, ...) \
    case opc: return strname;

                OPCODE(OP_MOVE, "move", LIT_INSTYP_ABC) /* R(A) := RC(B) */
                OPCODE(OP_LOADNULL, "ldnull", LIT_INSTYP_ABC) /* R(A) := null */
                OPCODE(OP_LOADBOOL, "ldbool", LIT_INSTYP_ABC) /* R(A) := (bool) B */
                OPCODE(OP_MAKECLOSURE, "mkclosure", LIT_INSTYP_ABX) /* R(A) := PrC[Bx] */
                OPCODE(OP_MAKEARRAY, "mkarray", LIT_INSTYP_ABX) /* R(A) := new Array(Bx) */
                OPCODE(OP_MAKEOBJECT, "mkobject", LIT_INSTYP_ABC) /* R(A) = new Object() */
                OPCODE(OP_MAKERANGE, "mkrange", LIT_INSTYP_ABC) /* R(A) = new Range(RC(B), RC(C)) */
                OPCODE(OP_RETURN, "return", LIT_INSTYP_ABC) /* return R(A) */
                OPCODE(OP_MATHADD, "m.add", LIT_INSTYP_ABC) /* R(A) := RC(B) + RC(C) */
                OPCODE(OP_MATHSUBTRACT, "m.subtr", LIT_INSTYP_ABC) /* R(A) := RC(B) - RC(C) */
                OPCODE(OP_MATHMULTIPLY, "m.mult", LIT_INSTYP_ABC) /* R(A) := RC(B) * RC(C) */
                OPCODE(OP_MATHDIVIDE, "m.div", LIT_INSTYP_ABC) /* R(A) := RC(B) / RC(C) */
                OPCODE(OP_MATHFLOORDIVIDE, "m.fldiv", LIT_INSTYP_ABC) /* R(A) := floor(RC(B) / RC(C)) */
                OPCODE(OP_MATHMOD, "m.mod", LIT_INSTYP_ABC) /* R(A) := RC(B) % RC(C) */
                OPCODE(OP_MATHPOWER, "m.pow", LIT_INSTYP_ABC) /* R(A) := pow(RC(B), RC(C)) */
                OPCODE(OP_MATHLEFTSHIFT, "m.shleft", LIT_INSTYP_ABC) /* R(A) := RC(B) << RC(C) */
                OPCODE(OP_MATHRIGHTSHIFT, "m.shright", LIT_INSTYP_ABC) /* R(A) := RC(B) >> RC(C) */
                OPCODE(OP_BINXOR, "m.bxor", LIT_INSTYP_ABC) /* R(A) := RC(B) ^ RC(C) */
                OPCODE(OP_BINAND, "m.band", LIT_INSTYP_ABC) /* R(A) := RC(B) & RC(C) */
                OPCODE(OP_BINOR, "m.bor", LIT_INSTYP_ABC) /* R(A) := RC(B) | RC(C) */
                OPCODE(OP_JUMP, "jump", LIT_INSTYP_ASBX) /* PC += sBx */
                OPCODE(OP_JUMPIFTRUE, "jumpiftrue", LIT_INSTYP_ABX) /* if (R(A)) PC += Bx */
                OPCODE(OP_JUMPIFFALSE, "jumpiffalse", LIT_INSTYP_ABX) /* if (not R(A)) PC += Bx */
                OPCODE(OP_JUMPIFNONNULL, "non_null_jump", LIT_INSTYP_ABX) /* if (R(A) != null) PC += Bx */
                OPCODE(OP_JUMPIFNULL, "null_jump", LIT_INSTYP_ABX) /* if (R(A) == null) PC += Bx */
                OPCODE(OP_EQUAL, "eq", LIT_INSTYP_ABC) /* R(A) := RC(B) == RC(C) */
                OPCODE(OP_LESSTHAN, "lessthan", LIT_INSTYP_ABC) /* R(A) := RC(B) < RC(C) */
                OPCODE(OP_LESSEQUAL, "lessequal", LIT_INSTYP_ABC) /* R(A) := RC(B) <= RC(C) */
                OPCODE(OP_GREATERTHAN, "greater", LIT_INSTYP_ABC) /* R(A) := RC(B) > RC(C) */
                OPCODE(OP_GREATEREQUAL, "greaterequal", LIT_INSTYP_ABC) /* R(A) := RC(B) >= RC(C) */
                OPCODE(OP_NEGATE, "negate", LIT_INSTYP_ABC) /* R(A) := -RC(B) */
                OPCODE(OP_NOT, "not", LIT_INSTYP_ABC) /* R(A) := !RC(B) */
                OPCODE(OP_BINNOT, "m.bnot", LIT_INSTYP_ABC) /* R(A) := ~RC(B) */
                OPCODE(OP_GLOBALSET, "globalset", LIT_INSTYP_ABX) /* G[C(A)] := RC(BX) */
                OPCODE(OP_GLOBALGET, "globalget", LIT_INSTYP_ABX) /* R(A) := G[C(Bx)] */
                OPCODE(OP_UPVALUESET, "upvset", LIT_INSTYP_ABX) /* U[A] := RC(Bx) */
                OPCODE(OP_UPVALUEGET, "upvget", LIT_INSTYP_ABX) /* R(A) := U[Bx] */
                OPCODE(OP_PRIVATESET, "privset", LIT_INSTYP_ABX) /* P[A] := RC(Bx) */
                OPCODE(OP_PRIVATEGET, "privget", LIT_INSTYP_ABX) /* R(A) := P[C(Bx)] */
                OPCODE(OP_CALLCALLABLE, "call", LIT_INSTYP_ABC) /* R(A) := R(A)(R(A + 1), ..., R(A + B - 1)) */
                OPCODE(OP_UPVALUECLOSE, "upvclose", LIT_INSTYP_ABC) /* close_upvalue(R(A)) */
                OPCODE(OP_CLASSMAKE, "mkclass", LIT_INSTYP_ABC) /* G[C(A)] = R[C] = new_class(C(A), C(B - 1)) */
                OPCODE(OP_CLASSPUTFIELDSTATIC, "classputstaticfield", LIT_INSTYP_ABC) /* R(A)[C(B)] = RC(C) */
                OPCODE(OP_CLASSPUTMETHOD, "classputmethod", LIT_INSTYP_ABC) /* R(A).Methods[C(B)] = RC(C) */
                OPCODE(OP_FIELDGET, "fieldget", LIT_INSTYP_ABC) /* R(A) = R(B)[C(C)] */
                OPCODE(OP_CLASSGETSUPERMETHOD, "classgetsupermethod", LIT_INSTYP_ABC) /* R(A) = R(B).super[C(C)] */
                OPCODE(OP_FIELDSET, "fieldset", LIT_INSTYP_ABC) /* R(A)[C(B)] = R(C) */
                OPCODE(OP_IS, "is", LIT_INSTYP_ABC) /* R(A) := RC(B) is G[C(C)] */
                OPCODE(OP_INVOKE, "classinvoke", LIT_INSTYP_ABC) /* R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1)) */
                OPCODE(OP_INVOKESUPER, "classinvokesuper", LIT_INSTYP_ABC) /* R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1)) */
                OPCODE(OP_SUBSCRIPTGET, "indexget", LIT_INSTYP_ABC) /* R(A) := R(A)[RC(B)] */
                OPCODE(OP_SUBSCRIPTSET, "indexset", LIT_INSTYP_ABC) /* R(A)[RC(B)] := R(C) */
                OPCODE(OP_ARRAYPUSH, "arraypush", LIT_INSTYP_ABX) /* R(A)[R(A).listcount++] = RC(Bx) */
                OPCODE(OP_OBJECTPUSH, "objectpush", LIT_INSTYP_ABC) /* R(A)[R(B)] = RC(C) */
                OPCODE(OP_REFGLOBAL, "refglobal", LIT_INSTYP_ABX) /* R(A) := ref G(C[Bx]) */
                OPCODE(OP_REFPRIVATE, "refprivate", LIT_INSTYP_ABX) /* R(A) := ref P(Bx) */
                OPCODE(OP_REFLOCAL, "reflocal", LIT_INSTYP_ABC) /* R(A) := ref R(B) */
                OPCODE(OP_REFUPVALUE, "refupvalue", LIT_INSTYP_ABX) /* R(A) := ref U(Bx) */
                OPCODE(OP_REFFIELD, "reffield", LIT_INSTYP_ABC) /* R(A) = ref R(B)[C(C)] */
                OPCODE(OP_REFSET, "setref", LIT_INSTYP_ABC) /* ref R(A) := R(B) */
            