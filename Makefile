.PHONY: test build-ui build-scene inspect-abi clean

PYTHON ?= python3

test:
	$(PYTHON) tests/run_all_tests.py

build-ui:
	$(PYTHON) scripts/build_ui_artifacts.py --out-dir build/arm-ui

build-scene:
	$(PYTHON) scripts/build_ui_artifacts.py --scene-only --out-dir build/arm-ui

inspect-abi:
	$(PYTHON) verifiers/inspect_ui_abi.py

clean:
	rm -rf build/ __pycache__/ menu-ui/__pycache__/ scripts/__pycache__/ tests/__pycache__/ verifiers/__pycache__/
