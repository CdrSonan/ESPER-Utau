namespace ESPER_Utau
{
    public static class PitchUtils
    {
        /// <summary>
        /// Converts MIDI pitch value to ESPER pitch format.
        /// </summary>
        /// <param name="pitch">MIDI pitch value</param>
        /// <param name="waveSampleRate">Sample rate of the audio</param>
        /// <returns>Pitch in ESPER format</returns>
        public static float MidiPitchToEsperPitch(float pitch, int waveSampleRate)
        {
            return waveSampleRate / (440 * float.Pow(2, (pitch - 69 + 12) / 12));
        }
    }
}