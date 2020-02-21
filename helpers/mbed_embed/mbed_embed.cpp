#if defined(__LP64__)
#error You probably want to build this in 32-bit mode.
#endif

#include <err.h>
#include <stdio.h>

#include <map>

#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>

#define PRINT_FIELD(var_name, x, name) print_field(var_name, #name, x.name)
#define PRINT_FIELD_PTR(var_name, x, name) print_field(var_name, #name, x->name)

static FILE* f;
static FILE* limbs;

static int indentation;
struct scoped_indent {
  scoped_indent() {
    ++indentation;
  }

  ~scoped_indent() {
    --indentation;
  }
};

void print_indent(FILE* fp = f) {
  for (int i = 0; i < indentation; ++i) {
    fprintf(fp, "  ");
  }
}

void print_field(const char* var_name, const char* field_name, int value) {
  print_indent();
  fprintf(f, ".%s = %d,\n", field_name, value);
}

void print_field(const char* var_name, const char* field_name, size_t value) {
  print_indent();
  fprintf(f, ".%s = %zu,\n", field_name, value);
}

void print_field(const char* var_name, const char* field_name, mbedtls_mpi& value) {
  print_indent();
  fprintf(f, ".%s = {\n", field_name);

  {
    scoped_indent _;
    PRINT_FIELD(var_name, value, s);
    PRINT_FIELD(var_name, value, n);
    print_indent();
    if (value.p) {
      fprintf(f, ".p = const_cast<mbedtls_mpi_uint*>(__limbs_%s_%s),\n", var_name, field_name);
      fprintf(limbs, "static const mbedtls_mpi_uint __limbs_%s_%s[%zu] = {\n", var_name, field_name, value.n);
      fprintf(limbs, "  ");
      for (size_t i = 0; i < value.n; ++i) {
        fprintf(limbs, "%lu, ", static_cast<unsigned long>(value.p[i]));
      }
      fprintf(limbs, "\n};\n");

    } else {
      fprintf(f, ".p = 0,\n");
    }
  }

  print_indent();
  fprintf(f, "},\n");
}

void print_array(const char* name, const unsigned char* data, size_t len) {
  print_indent(stdout);
  printf("const unsigned char %s[%zu] = {\n", name, len);
  {
    scoped_indent _;
    print_indent(stdout);
    for (size_t i = 0; i < len; ++i) {
      printf("0x%02x, ", data[i]);
    }
  }
  printf("\n};\n");
}

void print_definition(mbedtls_rsa_context* rsa, const unsigned char* ds4_serial,
                      const unsigned char* ds4_signature) {
  char* struct_buf = nullptr;
  size_t struct_len = 0;

  char* limbs_buf = nullptr;
  size_t limbs_len = 0;

  f = open_memstream(&struct_buf, &struct_len);
  limbs = open_memstream(&limbs_buf, &limbs_len);

  const char* name = "ds4_key";
  fprintf(f, "const struct mbedtls_rsa_context __%s = {\n", name);
  {
    scoped_indent _;
    PRINT_FIELD_PTR(name, rsa, ver);
    PRINT_FIELD_PTR(name, rsa, len);
    PRINT_FIELD_PTR(name, rsa, N);
    PRINT_FIELD_PTR(name, rsa, E);

    PRINT_FIELD_PTR(name, rsa, D);
    PRINT_FIELD_PTR(name, rsa, P);
    PRINT_FIELD_PTR(name, rsa, Q);

    PRINT_FIELD_PTR(name, rsa, DP);
    PRINT_FIELD_PTR(name, rsa, DQ);
    PRINT_FIELD_PTR(name, rsa, QP);

    PRINT_FIELD_PTR(name, rsa, RN);

    PRINT_FIELD_PTR(name, rsa, RP);
    PRINT_FIELD_PTR(name, rsa, RQ);

    PRINT_FIELD_PTR(name, rsa, Vi);
    PRINT_FIELD_PTR(name, rsa, Vf);

    print_indent();
    fprintf(f, ".padding = MBEDTLS_RSA_PKCS_V21,\n");

    print_indent();
    fprintf(f, ".hash_id = MBEDTLS_MD_SHA256,\n");
  }

  fprintf(f, "};\n");

  fclose(f);
  fclose(limbs);

  printf("%s\n%s", limbs_buf, struct_buf);

  unsigned char ds4_key_n[256];
  if (mbedtls_mpi_write_binary(&rsa->N, ds4_key_n, sizeof(ds4_key_n)) != 0) {
    errx(1, "failed to write ds4_key_n as big endian");
  }

  print_array("__ds4_key_n", ds4_key_n, sizeof(ds4_key_n));

  unsigned char ds4_key_e[256];
  if (mbedtls_mpi_write_binary(&rsa->E, ds4_key_e, sizeof(ds4_key_e)) != 0) {
    errx(1, "failed to write ds4_key_e as big endian");
  }

  print_array("__ds4_key_e", ds4_key_e, sizeof(ds4_key_e));

  print_array("__ds4_serial", ds4_serial, 16);
  print_array("__ds4_signature", ds4_signature, 256);
  printf("\n#define HAVE_DS4_KEY 1\n");
}

int main(int argc, char** argv) {
  if (argc != 4) {
    errx(1, "usage: mbed_embed PRIVATE_KEY_DER SERIAL SIGNATURE");
  }

  unsigned char der_buf[4096];
  size_t der_len;
  {
    FILE* f = fopen(argv[1], "r");
    if (!f) {
      err(1, "failed to open '%s'", argv[1]);
    }

    der_len = fread(der_buf, 1, sizeof(der_buf), f);
    if (der_len == 0) {
      errx(1, "failed to read from '%s'", argv[1]);
    }

    fclose(f);
  }

  unsigned char serial_buf[17];
  {
    FILE* f = fopen(argv[2], "r");
    if (!f) {
      err(1, "failed to open '%s'", argv[2]);
    }

    size_t len = fread(serial_buf, 1, sizeof(serial_buf), f);
    if (len != 16) {
      err(1, "failed to read from '%s': got %zu bytes", argv[2], len);
    }

    fclose(f);
  }

  unsigned char signature_buf[257];
  {
    FILE* f = fopen(argv[3], "r");
    if (!f) {
      err(1, "failed to open '%s'", argv[3]);
    }

    size_t len = fread(signature_buf, 1, sizeof(signature_buf), f);
    if (len != 256) {
      err(1, "failed to read from '%s': got %zu bytes", argv[3], len);
    }

    fclose(f);
  }

  mbedtls_pk_context pk_ctx;
  mbedtls_pk_init(&pk_ctx);

  int rc = mbedtls_pk_parse_key(&pk_ctx, der_buf, der_len, nullptr, 0);
  if (rc != 0) {
    errx(1, "failed to parse key");
  }

  if (mbedtls_pk_get_type(&pk_ctx) != MBEDTLS_PK_RSA) {
    errx(1, "invalid key format");
  }

  mbedtls_rsa_context* rsa = mbedtls_pk_rsa(pk_ctx);
  if (mbedtls_rsa_complete(rsa) != 0) {
    errx(1, "failed to complete RSA key");
  }

  // Populate RN.
  mbedtls_mpi temp;
  mbedtls_mpi_init(&temp);
  mbedtls_mpi_init(&rsa->RN);
  if (mbedtls_mpi_exp_mod(&temp, &rsa->E, &rsa->E, &rsa->N, &rsa->RN) != 0) {
    errx(1, "failed to populate RN");
  }

  print_definition(rsa, serial_buf, signature_buf);
}
