# Copyright (C) 1996-2015 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, see
# <http://www.gnu.org/licenses/>.

# Makefile for the math library.

subdir		:= math

include ../Makeconfig

# Installed header files.
headers		:= math.h bits/mathcalls.h bits/mathinline.h bits/huge_val.h \
		   bits/huge_valf.h bits/huge_vall.h bits/inf.h bits/nan.h \
		   fpu_control.h complex.h bits/cmathcalls.h fenv.h \
		   bits/fenv.h bits/fenvinline.h bits/mathdef.h tgmath.h \
		   bits/math-finite.h bits/math-vector.h \
		   bits/libm-simd-decl-stubs.h

# FPU support code.
aux		:= setfpucw fpu_control

# Build the -lm library.

extra-libs	:= libm
extra-libs-others = $(extra-libs)

libm-support = s_lib_version s_matherr s_signgam			\
	       fclrexcpt fgetexcptflg fraiseexcpt fsetexcptflg		\
	       ftestexcept fegetround fesetround fegetenv feholdexcpt	\
	       fesetenv feupdateenv t_exp fedisblxcpt feenablxcpt	\
	       fegetexcept
libm-calls = e_acos e_acosh e_asin e_atan2 e_atanh e_cosh e_exp e_fmod	\
	     e_hypot e_j0 e_j1 e_jn e_lgamma_r e_log e_log10 e_pow	\
	     e_rem_pio2 e_remainder e_scalb e_sinh e_sqrt e_gamma_r	\
	     e_ilogb							\
	     k_cos k_rem_pio2 k_sin k_tan s_asinh s_atan s_cbrt		\
	     s_ceil s_cos s_erf s_expm1 s_fabs				\
	     s_floor s_log1p w_log1p s_logb				\
	     s_nextafter s_nexttoward s_rint s_scalbln w_scalbln	\
	     s_significand s_sin s_tan s_tanh w_acos w_acosh w_asin	\
	     w_atan2 w_atanh w_cosh w_drem w_exp w_exp2 w_exp10 w_fmod	\
	     w_tgamma w_hypot w_j0 w_j1 w_jn w_lgamma w_lgamma_r	\
	     w_log w_log10 w_pow w_remainder w_scalb w_sinh w_sqrt	\
	     w_ilogb							\
	     s_fpclassify s_fmax s_fmin s_fdim s_nan s_trunc		\
	     s_remquo e_log2 e_exp2 s_round s_nearbyint s_sincos	\
	     conj cimag creal cabs carg s_cexp s_csinh s_ccosh s_clog	\
	     s_catan s_casin s_ccos s_csin s_ctan s_ctanh s_cacos	\
	     s_casinh s_cacosh s_catanh s_csqrt s_cpow s_cproj s_clog10 \
	     s_fma s_lrint s_llrint s_lround s_llround e_exp10 w_log2	\
	     s_isinf_ns s_issignaling $(calls:s_%=m_%) x2y2m1 k_casinh	\
	     gamma_product k_standard

dbl-only-routines := branred doasin dosincos halfulp mpa mpatan2	\
		     mpatan mpexp mplog mpsqrt mptan sincos32 slowexp	\
		     slowpow sincostab
libm-routines = $(strip $(libm-support) $(libm-calls) \
			$(patsubst %_rf,%f_r,$(libm-calls:=f))	\
			$(long-m-$(long-double-fcts))) \
		$(dbl-only-routines)
long-m-routines = $(patsubst %_rl,%l_r,$(libm-calls:=l))
long-m-support = t_sincosl k_sincosl
long-m-yes = $(long-m-routines) $(long-m-support)

# These functions are in libc instead of libm because __printf_fp
# calls them, so any program using printf will need them linked in,
# and we don't want to have to link every program with -lm.
# In libm-calls (above), list m_foo in place of s_foo for any
# routine that should be compiled separately for its libc and libm versions.
calls = s_isinf s_isnan s_finite s_copysign s_modf s_scalbn s_frexp s_ldexp \
	s_signbit
generated += $(foreach s,.c .S l.c l.S f.c f.S,$(calls:s_%=m_%$s))
routines = $(calls) $(calls:=f) $(long-c-$(long-double-fcts))
long-c-yes = $(calls:=l)

ifeq ($(build-mathvec),yes)
# We need to install libm.so as linker script
# for more comfortable use of vector math library.
install-lib-ldscripts := libm.so
install_subdir: $(inst_libdir)/libm.so
$(inst_libdir)/libm.so: $(common-objpfx)format.lds \
			$(libm) \
			$(libmvec) \
			$(+force)
	(echo '/* GNU ld script'; echo '*/';\
	 cat $<; \
	 echo 'GROUP ( $(slibdir)/libm.so$(libm.so-version) ' \
	      'AS_NEEDED ( $(slibdir)/libmvec.so$(libmvec.so-version) ) )' \
	) > $@
endif

# Rules for the test suite.
tests = test-matherr test-fenv atest-exp atest-sincos atest-exp2 basic-test \
	test-misc test-fpucw test-fpucw-ieee tst-definitions test-tgmath \
	test-tgmath-ret bug-nextafter bug-nexttoward bug-tgmath1 \
	test-tgmath-int test-tgmath2 test-powl tst-CMPLX tst-CMPLX2 test-snan \
	test-fenv-tls test-fenv-preserve test-fenv-return test-fenvinline \
	$(tests-static)
tests-static = test-fpucw-static test-fpucw-ieee-static
# We do the `long double' tests only if this data type is available and
# distinct from `double'.
test-longdouble-yes = test-ldouble test-ildoubl

ifneq (no,$(PERL))
libm-vec-tests = $(addprefix test-,$(libmvec-tests))
libm-tests = test-float test-double $(test-longdouble-$(long-double-fcts)) \
	test-ifloat test-idouble $(libm-vec-tests)
libm-tests.o = $(addsuffix .o,$(libm-tests))

tests += $(libm-tests)
libm-tests-generated = libm-test-ulps.h libm-have-vector-test.h libm-test.c
generated += $(libm-tests-generated) libm-test.stmp

# This is needed for dependencies
before-compile += $(objpfx)libm-test.c

ulps-file = $(firstword $(wildcard $(sysdirs:%=%/libm-test-ulps)))

$(addprefix $(objpfx), $(libm-tests-generated)): $(objpfx)libm-test.stmp

$(objpfx)libm-test.stmp: $(ulps-file) libm-test.inc gen-libm-test.pl \
			 gen-libm-have-vector-test.sh auto-libm-test-out
	$(make-target-directory)
	$(PERL) gen-libm-test.pl -u $< -o "$(objpfx)"
	$(SHELL) gen-libm-have-vector-test.sh > $(objpfx)libm-have-vector-test.h
	@echo > $@

$(objpfx)test-float.o: $(objpfx)libm-test.stmp
$(objpfx)test-ifloat.o: $(objpfx)libm-test.stmp
$(objpfx)test-double.o: $(objpfx)libm-test.stmp
$(objpfx)test-idouble.o: $(objpfx)libm-test.stmp
$(objpfx)test-ldouble.o: $(objpfx)libm-test.stmp
$(objpfx)test-ildoubl.o: $(objpfx)libm-test.stmp
endif

libm-test-fast-math-cflags = -fno-builtin -D__FAST_MATH__ -DTEST_FAST_MATH
libm-test-vec-cflags = $(libm-test-fast-math-cflags) -fno-inline \
		       -ffloat-store -D_OPENMP=201307 -Wno-unknown-pragmas

CFLAGS-test-double-vlen2.c = $(libm-test-vec-cflags)

CFLAGS-test-double-vlen4.c = $(libm-test-vec-cflags)
CFLAGS-test-double-vlen4-wrappers.c = $(double-vlen4-arch-ext-cflags)

CFLAGS-test-double-vlen8.c = $(libm-test-vec-cflags)
CFLAGS-test-double-vlen8-wrappers.c = $(double-vlen8-arch-ext-cflags)

CFLAGS-test-float-vlen4.c = $(libm-test-vec-cflags)

CFLAGS-test-float-vlen8.c = $(libm-test-vec-cflags)
CFLAGS-test-float-vlen8-wrappers.c = $(float-vlen8-arch-ext-cflags)

CFLAGS-test-float-vlen16.c = $(libm-test-vec-cflags)
CFLAGS-test-float-vlen16-wrappers.c = $(float-vlen16-arch-ext-cflags)

CFLAGS-test-float.c = -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-double.c = -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-ldouble.c = -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-tgmath.c = -fno-builtin
# The following testcase uses very long lines (>3 million), so it may take a
# while to compile it. See: http://llvm.org/bugs/show_bug.cgi?id=14106 and
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=54402
CFLAGS-test-tgmath2.c = -fno-builtin
CFLAGS-test-tgmath-ret.c = -fno-builtin
CFLAGS-test-powl.c = -fno-builtin
CPPFLAGS-test-ifloat.c = -U__LIBC_INTERNAL_MATH_INLINES \
			 $(libm-test-fast-math-cflags)
CPPFLAGS-test-idouble.c = -U__LIBC_INTERNAL_MATH_INLINES \
			  $(libm-test-fast-math-cflags)
CPPFLAGS-test-ildoubl.c = -U__LIBC_INTERNAL_MATH_INLINES \
			  $(libm-test-fast-math-cflags)


# The -lieee module sets the _LIB_VERSION_ switch to IEEE mode
# for error handling in the -lm functions.
install-lib += libieee.a
non-lib.a += libieee.a
extra-objs += libieee.a ieee-math.o

include ../Rules

ifneq (no,$(PERL))
# This must come after the inclusion of sysdeps Makefiles via Rules.
$(addprefix $(objpfx), $(addsuffix .o, $(libm-vec-tests))): $(objpfx)libm-test.stmp

# Run the math programs to automatically generate ULPs files.
.PHONY: regen-ulps

run-regen-ulps = $(test-wrapper-env) \
	    $(run-program-env) \
	    $($*-ENV) $(rtld-prefix) $${run}

regen-ulps: $(addprefix $(objpfx),$(libm-tests))
	rm -f $(objpfx)ULPs; rm -f $(objpfx)NewUlps; \
	cp $(ulps-file) $(objpfx)libm-test-ulps; \
	for run in $^; do \
	  echo "Regenerating ULPs for $${run}"; \
	  $(run-regen-ulps) -u -o $(objpfx); \
	  cat $(objpfx)ULPs >> $(objpfx)libm-test-ulps; \
	  rm $(objpfx)ULPs; \
	done; \
	$(PERL) gen-libm-test.pl -o $(objpfx) -n -u $(objpfx)libm-test-ulps; \
	echo "Automatic regeneration of ULPs complete."; \
	echo "Difference between the current baseline and the new baseline is:";\
	diff -urN $(ulps-file) $(objpfx)NewUlps; \
	echo "Copy $(objpfx)NewUlps to $(ulps-file) (relative to source)."
else
regen-ulps:
	@echo "Automatic regeneration of ULPs requires perl."; \
	exit 1;
endif

# The generated sysd-rules file defines rules like this for sources
# coming from sysdeps/ directories.  These rules find the generic sources.
define o-iterator-doit
$(objpfx)m_%$o: s_%.c $(before-compile); $$(compile-command.c)
endef
object-suffixes-left := $(all-object-suffixes)
include $(o-iterator)


# This file defines the default _LIB_VERSION variable that controls
# the error return conventions for the math functions.
CPPFLAGS-s_lib_version.c := -D_POSIX_MODE

# We don't want the fdlibm code to use the inline math functions,
# only the fdlibm code.
math-CPPFLAGS += -D__NO_MATH_INLINES -D__LIBC_INTERNAL_MATH_INLINES

ifneq ($(long-double-fcts),yes)
# The `double' and `long double' types are the same on this machine.
# We won't compile the `long double' code at all.  Tell the `double' code
# to define aliases for the `FUNCl' names.  To avoid type conflicts in
# defining those aliases, tell <math.h> to declare the `FUNCl' names with
# `double' instead of `long double'.
math-CPPFLAGS += -DNO_LONG_DOUBLE -D_Mlong_double_=double
endif

# The fdlibm code generates a lot of these warnings but is otherwise clean.
override CFLAGS += -Wno-uninitialized

# The -lieee library is actually an object file.
# The module just defines the _LIB_VERSION_ variable.
# It's not a library to make sure it is linked in instead of s_lib_version.o.
$(objpfx)libieee.a: $(objpfx)ieee-math.o
	rm -f $@
	$(patsubst %/,cd % &&,$(objpfx)) \
	$(LN_S) $(<F) $(@F)

$(addprefix $(objpfx),$(filter-out $(tests-static) $(libm-vec-tests),$(tests))): $(libm)
$(addprefix $(objpfx),$(tests-static)): $(objpfx)libm.a
$(addprefix $(objpfx), $(libm-vec-tests)): $(objpfx)%: $(libm) $(libmvec) \
					   $(objpfx)init-arch.o $(objpfx)%-wrappers.o

gmp-objs = $(patsubst %,$(common-objpfx)stdlib/%.o,\
		      add_n sub_n cmp addmul_1 mul_1 mul_n divmod_1 \
		      lshift rshift mp_clz_tab udiv_qrnnd inlines \
		      $(gmp-sysdep_routines))
$(objpfx)atest-exp: $(gmp-objs)
$(objpfx)atest-sincos: $(gmp-objs)
$(objpfx)atest-exp2: $(gmp-objs)
$(objpfx)test-fenv-tls: $(shared-thread-library)
