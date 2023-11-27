package mad.location.manager.lib.Interfaces;

import android.location.Location;

import org.json.JSONException;

import java.io.IOException;

/**
 * Created by lezh1k on 2/13/18.
 */

public interface LocationServiceInterface {
    void locationChanged(Location location);
}
