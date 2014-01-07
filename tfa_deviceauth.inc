<?php

class TfaDeviceAuth extends TfaPlugin implements TfaLoginPlugin {

  public function loginAllowed() {
    // check if cookie is present
    // return true if so
    // @todo
    if ($this->context['uid'] === '141') {
      return TRUE;
    }
    return FALSE;
  }

  /*public function entryForm() {
    // add "remember this device" form field
  }

  public function submitForm() {
    // if "remember this device" is set
    // set a cookie for this device
  }*/
}
