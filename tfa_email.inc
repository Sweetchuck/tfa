<?php

class TfaEmail implements TfaApiPlugin {

  public function __construct() {
  }

  public function loginAllowed() {
    // not needed
  }

  public function beginProcess() {
    // generate code
    // store code
    // send email with code
  }

  public function entryForm() {
    // code input
    // resend code
  }

  public function validateForm() {
    // compare code
  }

  public function submitForm() {
    // set context to validated
  }
}
