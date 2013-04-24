ENGINE_DIR = ./engine/
SHALLOW_DIR = ./shallow/

subsystem:
	$(MAKE) -C $(ENGINE_DIR)
	$(MAKE) -C $(SHALLOW_DIR)
	
.PHONY: clean
clean:
	$(MAKE) -C $(ENGINE_DIR) clean
	$(MAKE) -C $(SHALLOW_DIR) clean
