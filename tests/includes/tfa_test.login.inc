<?php

class TfaTestLogin extends TfaBasePlugin implements TfaLoginPluginInterface {

  public function loginAllowed() {
    if ($this->context['uid'] === variable_get('tfa_test_login_uid', '')) {
      return TRUE;
    }
    return FALSE;
  }
}
