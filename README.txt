Two-factor Authentication for Drupal

INSTALLATION:

For default use of the TFA module using the SMS Framework follow these steps.

 * Install SMS Framework module and choose an available gateway
 * Install TFA module
 * Enable Profile module and add a profile field for storing a user's phone number
 * Visit Administration > Settings > TFA to choose the Profile field for use

Optionally you can allow certain user roles to skip the TFA process by giving the 'skip tfa' permission at Administration > User management > Permissions.

Note, if no profile field is enabled or populated for a user the TFA process will not be started.

API:

Consult tfa.api.php for directions on extending TFA with different channels and storage.
