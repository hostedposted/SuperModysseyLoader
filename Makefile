
.PHONY: all clean

export PU_MAJOR := 0
export PU_MINOR := 3
export PU_MICRO := 0	

all:
	@$(MAKE) -C Plutonium/
	@$(MAKE) -C SuperModysseyLoader/

clean:
	@$(MAKE) clean -C Plutonium/
	@$(MAKE) clean -C SuperModysseyLoader/