/* libclass.c */
void lit_class_inherit(LitState *state, LitClass *selfclass, LitClass *other);
void lit_class_bindmethod(LitState *state, LitClass *selfclass, const char *name, LitNativeMethodFn fn);
void lit_class_bindprimitive(LitState *state, LitClass *selfclass, const char *name, LitPrimitiveMethodFn fn);
void lit_class_bindconstructor(LitState *state, LitClass *selfclass, LitNativeMethodFn fn);
void lit_class_bindstaticmethod(LitState *state, LitClass *selfclass, const char *name, LitNativeMethodFn fn);
void lit_class_bindstaticprimitive(LitState *state, LitClass *selfclass, const char *name, LitPrimitiveMethodFn fn);
void lit_class_setstaticfield(LitState *state, LitClass *selfclass, const char *name, LitValue val);
void lit_class_bindgetsetter(LitState *state, LitClass *selfclass, const char *name, LitNativeMethodFn fnget, LitNativeMethodFn fnset);
