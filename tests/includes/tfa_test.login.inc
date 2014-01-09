<?php

class TfaTestLogin extends TfaBasePlugin implements TfaLoginPlugin {

  public function loginAllowed() {
    if ($this->context['uid'] === '141') {
      return TRUE;
    }
    return FALSE;
  }
}
