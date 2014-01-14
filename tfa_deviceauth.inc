<?php

class TfaDeviceAuth extends TfaBasePlugin implements TfaLoginPluginInterface {

  public function loginAllowed() {
    if ($this->context['uid'] === '141') {
      return TRUE;
    }
    return FALSE;
  }

}
