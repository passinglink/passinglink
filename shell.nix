with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "passinglink";
  nativeBuildInputs = [
    libusb
  ];
  LD_LIBRARY_PATH="${pkgs.libusb}/lib";

  shellHook = ''
    SOURCE_DATE_EPOCH=$(date +%s)
    source .venv/bin/activate 2>/dev/null || (
      python3 -m venv .venv &&
      .venv/bin/pip install nrfutil pyocd west cryptography cbor
    )
    export PATH=$PWD/.venv/bin:$PATH
  '';
}
