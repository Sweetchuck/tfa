<?php

class TfaDeviceAuth implements TfaApiPlugin {

  public function __construct() {
  }

  public function loginAllowed() {
    // check if cookie is present
    // return true if so
  }

  public function entryForm() {
    // add "remember this device" form field
  }

  public function submitForm() {
    // if "remember this device" is set
    // set a cookie for this device
  }
}
