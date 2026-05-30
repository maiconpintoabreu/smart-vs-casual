#include "raymob.h"

static Callback onPause = NULL;
static Callback onResume = NULL;

void SetOnResumeCallBack(Callback callback){
    onResume = callback;
}
void SetOnPauseCallBack(Callback callback){
    onPause = callback;
}

JNIEXPORT void JNICALL
Java_ovh_maicongamedev_antistressparticles_NativeLoader_onWindowsFocused(JNIEnv *env, jobject obj) {
    if(onResume) onResume();
}

JNIEXPORT void JNICALL
Java_ovh_maicongamedev_antistressparticles_NativeLoader_onWindowsUnFocused(JNIEnv *env, jobject obj) {
    if(onPause) onPause();
}