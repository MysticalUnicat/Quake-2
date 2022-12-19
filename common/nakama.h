#ifndef NAKAMA_C_STUB
#define NAKAMA_C_STUB

#include <alias/memory.h>
#include <uv.h>

struct NakamaClient_Parameters {
  const char *server_key;
  const char *host;
  int32_t port;
  bool ssl;

  alias_MemoryCB *mem_cb;

  uv_loop_t *loop;
};

struct NakamaClient_AuthParameters {
  enum {
    NakamaClient_AuthParameters_Device,
    NakamaClient_AuthParameters_Email,
    NakamaClient_AuthParameters_Steam
  } kind;
  bool create;
  bool import_friends;
  union {
    struct {
      const char *device_id;
    } device;
    struct {
      const char *email;
      const char *password;
      const char *username;
    } email;
    struct {
      const char *token;
      const char *username;
    } steam;
  }
};

struct NakamaClient;
struct NakamaSession;

typedef void (*NakamaClient_AuthErrorCallback)(struct NakamaClient *client, const char *error);
typedef void (*NakamaClient_AuthSuccessCallback)(struct NakamaClient *client, struct NakamaSession *session);

bool create_NakamaClient(struct NakamaClient_Parameters *parameters, struct NakamaClient **out_client);

void NakamaClient_set_user_pointer(struct NakamaClient *client, void *ud);
void *NakamaClient_get_user_pointer(struct NakamaClient *client);

bool NakamaClient_authenticate(struct NakamaClient *client, const struct NakamaClient_AuthParameters *parameters,
                               NakamaClient_AuthErrorCallback error_cb, NakamaClient_AuthSuccessCallback success_cb);

#endif