
#include <Greenhouse.h>
#include <vector>
#include <iostream>
#include <algorithm>

#include <libBasement/ob-hook-troves.h>
#include <libBasement/QuadraticVect.h>

using namespace oblong::greenhouse;

// super magic values for the arrow keys
// also handy for hooking a Greenhouse
// application up to a MaKey MaKey.
const char *UP    = "\xef\x9c\x80";
const char *DOWN  = "\xef\x9c\x81";
const char *LEFT  = "\xef\x9c\x82";
const char *RIGHT = "\xef\x9c\x83";


#define WAIT_BEFORE_DISPLAY 300.0
#define TEXT_VISIBLE_ALPHA .75

class Instructions  :  public Image {
public:
  Instructions (Str const& s)  :  Image () {
    SetName ("instructions");
    Text *t = new Text (s);
    t -> SetFontSize (t -> FontSize () * 0.4);
    SetCornerRadius (t -> Height () / 4.0);
    SetBackingColor (ObColor (0.0, 0.8));
    AppendKid (t);
    t -> SetAlignmentCenter ();
    SetWidth (t -> Width () * 1.1);
    SetHeight (t -> Height () * 1.1);
    SetAlpha (TEXT_VISIBLE_ALPHA);
    SetFireTimer (WAIT_BEFORE_DISPLAY);
  }

  void Fired () { Display (); }
  void Blurt (BlurtEvent *e) { Dismiss (); }

  void Display () { SetAlpha (TEXT_VISIBLE_ALPHA); }
  void Dismiss () {
    SetAlpha (.0);
    ResetTimerAndAnimation ();
  }

  virtual void SwipeUp (BlurtEvent *) { Dismiss (); }
  virtual void SwipeDown (BlurtEvent *) { Dismiss (); }
  virtual void SwipeLeft (BlurtEvent *) { Dismiss (); }
  virtual void SwipeRight (BlurtEvent *) { Dismiss (); }
  virtual void PointingHarden (PointingEvent *) { Dismiss (); }
  virtual void PointingSoften (PointingEvent *) { Dismiss (); }
};

class Vid  :  public Video {
private:
  bool active;
  float64 last_timestamp;
  size_t same_count;

  SINGLE_ARG_HOOK_MACHINERY (Done, Vid*);
public:
  Vid (const Str &path, const Str &name)  :  Video (path, false),
                                             active (false),
                                             last_timestamp (-1.),
                                             same_count (0)
  {
    Text *t = new Text (name);
    Vect trans (0.0, -Height () / 2.0 - t -> Height (), 0.0);
    t -> SetAlignmentCenter ();
    t -> SetTranslation (trans);
    AppendKid (t);
    SetShouldInhale (true);
  }

  void Blurt (BlurtEvent *e) {
    if (! active)
      return;

    if (Utters(e, "p") || Utters (e, " "))
      TogglePlay ();
    else if (Utters (e, "R") || Utters (e, DOWN)) {
      Stop ();
      Rewind ();
    }
  }

  void SetActive (const bool b) { active = b; same_count = 0; last_timestamp = -1; }
  const bool Active () const { return active; }

  virtual ObRetort Inhale (Atmosphere *atmo) {
    ObRetort tort = super::Inhale (atmo);
    if (tort . IsSplend () && active) {
      if (Timestamp () >= Duration ()) {
        Stop ();
        Rewind ();
        CallDoneHooks (this);
      }
    }
    return tort;
  }

  // If you harden/soften twice within a quarter second, treat that as "stop & rewind"
  // rather than toggling play/pause.
  #define STOP_TIME_THRESHOLD 0.25

  virtual void PointingSoften (PointingEvent *) {
    if (active) {
      if (STOP_TIME_THRESHOLD > CurTime ()) {
        Stop ();
        Rewind ();
      } else {
        TogglePlay ();
      }
      ZeroTime ();
    }
  }
};

static const float MOVE_FACTOR = 50.0;

struct VidDesc { Str path, name; float64 volume; };

class Vids  :  public Thing {
private:
  std::vector<Vid*> vids;
  size_t index;

  bool pushed_back;
  float pushback_min;
  Vect fist_begin;

  void SetRelTrans (const Vect &v) {
    SetTranslation (Feld () -> Loc () + v);
  }


  const Vect RelTrans ()  {
    return TranslationGoal () - Feld () -> Loc ();
  }

  void UpdateTranslation () {
    float wid = vids[index] -> Width () ;
    Vect tg = RelTrans ();
    float z_value = 0.0;
    if (tg.z < pushback_min / 2.0) {
      z_value = pushback_min;
    }
    Vect move = Vect ((wid + MOVE_FACTOR) * index * -1.0, 0.0, z_value);
    SetRelTrans (move);
  }

  void SetIndex (const size_t idx) {
    vids[index] -> SetActive(false);
    vids[index] -> Stop ();
    index = idx;
    vids[index] -> SetActive (true);
    UpdateTranslation ();
  }


  ObRetort VidDone (Vid*) {
    SetIndex ((1 + index) % vids . size ());
    vids[index] -> Rewind ();
    vids[index] -> Play ();
    return OB_OK;
  }

  void Move (const int direction) {
    const int ni = index + direction;
    if (0 <= ni && ni < vids.size ()) {
      SetIndex (ni);
    }
  }

  void TogglePushback () {
    pushed_back = !pushed_back;
    Vect v = RelTrans ();
    v.z = pushed_back ? pushback_min : 0.0;
    SetRelTrans (v);
  }

  void MoveLeft () { Move (-1); }
  void MoveRight () { Move (1); }

public:
  Vids (std::vector<VidDesc> const & vid_paths)  :
    Thing (), index (0), pushed_back (false), pushback_min (-1000.0)
  {
    for (const VidDesc &vdesc : vid_paths) {
      Vid *vid = new Vid (vdesc.path, vdesc.name);
      vid -> AppendDoneHook (&Vids::VidDone, this, "vid-done");
      AppendKid (vid);
      vid -> SetTranslation (Vect (vids . size () *
                                   (vid -> Width () + MOVE_FACTOR), 0.0, 0.0));
      vid -> SetCornerRadius (0.0);
      vid -> SetVolume (vdesc.volume);
      vids.push_back(vid);
    }
    if (0 < vids.size()) {
      vids[index] -> SetActive (true);
    }
    InstallTranslation (new QuadraticVect ());
    SetRelTrans(Vect (0.0, 0.0, 0.0));
  }

  void Blurt (BlurtEvent *e) {
    if (Utters (e, "a") || Utters (e, LEFT)) MoveLeft ();
    else if (Utters (e, "d") || Utters (e, RIGHT)) MoveRight ();
    else if (Utters (e, UP) || Utters (e, "s")) TogglePushback ();
  }

  virtual void SwipeLeft (BlurtEvent *) { MoveRight (); }
  virtual void SwipeRight (BlurtEvent *) { MoveLeft (); }
  virtual void SwipeUp (BlurtEvent *) {
    if (! pushed_back)
      TogglePushback ();
  }
  virtual void SwipeDown (BlurtEvent *) {
    if (pushed_back)
      TogglePushback ();
  }
};


static std::vector<VidDesc> ParseConfigFile (Str const &filename) {
  std::vector<VidDesc> videos;

  ObRetort tort;
  Slaw conf = Slaw::FromFile (filename, &tort);
  if (tort . IsSplend ()) {
    const Slaw &ingests = conf . Ingests ();
    const Slaw conf_vids = ingests . MapFind ("videos");
    const int64 N = conf_vids . Count ();
    Str vid_filename, vid_title;
    float64 vid_audio_level;
    for (int64 i = 0; i < N; ++i) {
      Slaw s = conf_vids . Nth (i);
      if (s . MapFind ("file") . Into (vid_filename) &&
          s . MapFind ("title") . Into (vid_title) &&
          s . MapFind ("audio") . Into (vid_audio_level))
        videos . push_back ({ vid_filename, vid_title, vid_audio_level });
    }
  }

  return videos;
}

void Setup () {
  HideNeedlePoints ();
  SetFeldsColor (Color (0.0, 0.0, 0.0));
  if (0 < args . Count ()) {
    std::vector<VidDesc> videos = ParseConfigFile (args[0]);
    Vids *vids = new Vids (videos);
    vids -> SlapOnFeld ();
    Instructions *t = new Instructions ("Activate features by closing the connection\n"
                                        "between ground (G) and one other button.");
    SpaceFeld *main = Feld ("main");
    t -> SlapOnFeld ();
    t -> SetTranslation (main -> Loc () + Vect (.0, main -> Height () / 3.0, .0));
  } else {
    Text *t = new Text ("No videos available");
    t -> SlapOnFeld ();
  }
}
