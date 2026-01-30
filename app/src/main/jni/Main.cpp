#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/Utils.h"
#include "Includes/obfuscate.h"
#include "KittyMemory/MemoryPatch.h"
#include "Menu/Setup.h"
#include <android/log.h>
#include <cmath>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <jni.h>
#include <list>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define targetLibName OBFUSCATE("libil2cpp.so")
#define LOG_TAG "Features"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool isCoolDown;
float attackInput, critRateInput, critDmgInput, skillDmgInput;
int sliderAtkSpd = 1, sliderMovSpd = 1;
void *instanceBtn;

float (*old_get_damage)(void *instance);
float get_damage(void *instance) {
  if (instance != nullptr && attackInput > 1) {
    float originalDMG = old_get_damage(instance);
    return originalDMG * attackInput;
  }
  return old_get_damage(instance);
}

float (*old_get_critRate)(void *instance);
float get_critRate(void *instance) {
  if (instance != nullptr && critRateInput > 1) {
    float originalCR = old_get_critRate(instance);
    float additionalCR = critRateInput / 100.0f;
    return (float)originalCR + additionalCR;
  }
  return old_get_critRate(instance);
}

float (*old_get_critDamageRate)(void *instance);
float get_critDamageRate(void *instance) {
  if (instance != nullptr && critDmgInput > 1) {
    return old_get_critDamageRate(instance) + critDmgInput;
  }
  return old_get_critDamageRate(instance);
}

float (*old_GetDamage)(void *instance);
float GetDamage(void *instance) {
  if (instance != nullptr && skillDmgInput > 1) {
    float originalSkillDMG = old_GetDamage(instance);
    float modifiedSkillDMG = originalSkillDMG * (float)skillDmgInput;
    return modifiedSkillDMG;
  }
  return old_GetDamage(instance);
}

float (*old_get_attackSpeed)(void *instance);
float get_attackSpeed(void *instance) {
  if (instance != nullptr && sliderAtkSpd > 1) {
    float originalAtkSpd = old_get_attackSpeed(instance);
    return originalAtkSpd + (float)sliderAtkSpd;
  }
  return old_get_attackSpeed(instance);
}

float (*old_get_moveSpeed)(void *instance);
float get_moveSpeed(void *instance) {
  if (instance != nullptr && sliderMovSpd > 1) {
    float originalMovSpd = old_get_moveSpeed(instance);
    return originalMovSpd + (float)sliderMovSpd;
  }
  return old_get_moveSpeed(instance);
}

void *hack_thread(void *) {
  LOGI(OBFUSCATE("pthread created"));

  do {
    sleep(1);
  } while (!isLibraryLoaded(targetLibName));

  LOGI(OBFUSCATE("%s has been loaded"), (const char *)targetLibName);

#if defined(__aarch64__)

#else

  HOOK("0x18E4534", get_damage, old_get_damage);
  HOOK("0x18E477C", get_critRate, old_get_critRate);
  HOOK("0x18E4840", get_critDamageRate, old_get_critDamageRate);
  HOOK("0x10141AC", GetDamage, old_GetDamage);
  HOOK("0x18E46A8", get_attackSpeed, old_get_attackSpeed);
  HOOK("0x18E5054", get_moveSpeed, old_get_moveSpeed);

  LOGI(OBFUSCATE("Done"));
#endif

  return nullptr;
}

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
  jobjectArray ret;

  const char *features[] = {OBFUSCATE("0_Toggle_NoSkill CD"),
                            OBFUSCATE("1_InputValue_1000_HeroAtk Multiplier"),
                            OBFUSCATE("2_InputValue_1000_CritRate+"),
                            OBFUSCATE("3_InputValue_1000_CritDmg+"),
                            OBFUSCATE("4_InputValue_1000_SkillDmg Multiplier"),
                            OBFUSCATE("5_SeekBar_AtkSpd+_1_20"),
                            OBFUSCATE("6_SeekBar_MovSpd+_1_20")};

  int Total_Feature = (sizeof features / sizeof features[0]);
  ret = (jobjectArray)env->NewObjectArray(
      Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
      env->NewStringUTF(""));

  for (int i = 0; i < Total_Feature; ++i)
    env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

  return (ret);
}

void Changes(JNIEnv *env, jclass clazz, jobject obj, jint featNum,
             jstring featName, jint value, jboolean boolean, jstring str) {

  LOGD(OBFUSCATE(
           "Feature name: %d - %s | Value: = %d | Bool: = %d | Text: = %s"),
       featNum, env->GetStringUTFChars(featName, nullptr),
       static_cast<int>(value), boolean,
       str != nullptr ? env->GetStringUTFChars(str, nullptr) : "");

  switch (featNum) {
  case 0:
    isCoolDown = boolean;
    PATCH_LIB_SWITCH("libil2cpp.so", "0x18E4904", "00 00 A0 E3 1E FF 2F E1",
                     boolean);
    break;
  case 1:
    attackInput = value;
    break;
  case 2:
    critRateInput = value;
    break;
  case 3:
    critDmgInput = value;
    break;
  case 4:
    skillDmgInput = value;
    break;
  case 5:
    if (value >= 1) {
      sliderAtkSpd = value;
    }
    break;
  case 6:
    if (value >= 1) {
      sliderMovSpd = value;
    }
    break;
  }
}

__attribute__((constructor)) void lib_main() {
  pthread_t ptid;
  pthread_create(&ptid, nullptr, hack_thread, nullptr);
}

int RegisterMenu(JNIEnv *env) {
  JNINativeMethod methods[] = {
      {OBFUSCATE("Icon"), OBFUSCATE("()Ljava/lang/String;"),
       reinterpret_cast<void *>(Icon)},
      {OBFUSCATE("IconWebViewData"), OBFUSCATE("()Ljava/lang/String;"),
       reinterpret_cast<void *>(IconWebViewData)},
      {OBFUSCATE("IsGameLibLoaded"), OBFUSCATE("()Z"),
       reinterpret_cast<void *>(isGameLibLoaded)},
      {OBFUSCATE("Init"),
       OBFUSCATE("(Landroid/content/Context;Landroid/widget/TextView;Landroid/"
                 "widget/TextView;)V"),
       reinterpret_cast<void *>(Init)},
      {OBFUSCATE("SettingsList"), OBFUSCATE("()[Ljava/lang/String;"),
       reinterpret_cast<void *>(SettingsList)},
      {OBFUSCATE("GetFeatureList"), OBFUSCATE("()[Ljava/lang/String;"),
       reinterpret_cast<void *>(GetFeatureList)},
  };

  jclass clazz = env->FindClass(OBFUSCATE("com/android/support/Menu"));
  if (!clazz)
    return JNI_ERR;
  if (env->RegisterNatives(clazz, methods,
                           sizeof(methods) / sizeof(methods[0])) != 0)
    return JNI_ERR;
  return JNI_OK;
}

int RegisterPreferences(JNIEnv *env) {
  JNINativeMethod methods[] = {
      {OBFUSCATE("Changes"),
       OBFUSCATE("(Landroid/content/Context;ILjava/lang/String;IZLjava/lang/"
                 "String;)V"),
       reinterpret_cast<void *>(Changes)},
  };
  jclass clazz = env->FindClass(OBFUSCATE("com/android/support/Preferences"));
  if (!clazz)
    return JNI_ERR;
  if (env->RegisterNatives(clazz, methods,
                           sizeof(methods) / sizeof(methods[0])) != 0)
    return JNI_ERR;
  return JNI_OK;
}

int RegisterMain(JNIEnv *env) {
  JNINativeMethod methods[] = {
      {OBFUSCATE("CheckOverlayPermission"),
       OBFUSCATE("(Landroid/content/Context;)V"),
       reinterpret_cast<void *>(CheckOverlayPermission)},
  };
  jclass clazz = env->FindClass(OBFUSCATE("com/android/support/Main"));
  if (!clazz)
    return JNI_ERR;
  if (env->RegisterNatives(clazz, methods,
                           sizeof(methods) / sizeof(methods[0])) != 0)
    return JNI_ERR;

  return JNI_OK;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  JNIEnv *env;
  vm->GetEnv((void **)&env, JNI_VERSION_1_6);
  if (RegisterMenu(env) != 0)
    return JNI_ERR;
  if (RegisterPreferences(env) != 0)
    return JNI_ERR;
  if (RegisterMain(env) != 0)
    return JNI_ERR;
  return JNI_VERSION_1_6;
}
