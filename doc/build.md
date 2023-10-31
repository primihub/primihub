# Build

## Choice PIR support
 
 Use microsoft-apsi definition in `.bazelrc` file.

if use microsoft-apsi PIR support, use

```
build:linux --define microsoft-apsi=true
```

or use openminded PIR library default.

### Microsoft APSI dependencies that need to be installed manually
***It is recommended to install with homebrew***
 
 * flatbuffers 2.0.8
 * tclap 1.2.5
 * zeromq 4.3.4
 * cppzmq 4.8.1

```
brew install flatbuffers tclap zeromq cppzmq
```