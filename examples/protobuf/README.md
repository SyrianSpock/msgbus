# Protobuf prototype

## requirements

* Google protobuf, installed for **Python 2**
* Nanopb definitions.
    Can be installed by running `pushd nanopb/generator/proto && make && popd`.

## building the message definitions

```
protoc --plugin=protoc-gen-nanopb=nanopb/generator/protoc-gen-nanopb simple.proto --nanopb_out=generated
```
