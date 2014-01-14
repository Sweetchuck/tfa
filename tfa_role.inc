<?php

class TfaRole extends TfaBasePlugin implements TfaLoginPluginInterface {

  public function loginAllowed() {
    if (isset($this->context['uid'])) {
      $account = user_load($this->context['uid']);
      if (in_array('test', $account->roles)) {
        return TRUE;
      }
    }
    return FALSE;
  }
}
