#os_arch=$(shell uname -m)
##flag_compile=$(shell if [ "$(os_arch)" == "x86_64"  ]; then "--cxxopt=-D_AMD64_"; fi)
#flag_compile=$(shell if [ "$(os_arch)" == "arm64" ]; then "--cxxopt=-D_ARM64_"; fi)
all:
	#bazel build --config=linux  //:node //:cli --sandbox_debug
	bazel build --cxxopt=-D_AMD64_ --config=linux  --define microsoft-apsi=true //:node //:cli --sandbox_debug
	#@echo $(flag_compile)

