; weak_b: a weak external 'maybe_get' whose alias tag is real_get -
; PUBLIC of an extern value produces IMAGE_SYM_CLASS_WEAK_EXTERNAL
format MS64 NEWCOFF
extrn real_get
public real_get as 'maybe_get'
