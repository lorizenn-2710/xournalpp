/*
 * Xournal++
 *
 * Dash definition of a stroke
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <vector>

#include "util/serializing/Serializable.h"  // for Serializable

class ObjectInputStream;
class ObjectOutputStream;


class LineStyle: public Serializable {
public:
    LineStyle();
    ~LineStyle() override;

    bool operator==(const LineStyle& other) const;

public:
    // Serialize interface
    void serialize(ObjectOutputStream& out) const override;
    void readSerialized(ObjectInputStream& in) override;

    /**
     * Get dash vector
     *
     * @return dashes
     */
    const std::vector<double>& getDashes() const;

    /**
     * @return true if dashed
     */
    bool hasDashes() const;

    /**
     * Set the dash vector and count
     *
     * @param dashes Dash data, will be moved, and continuous use from caller invalid
     */
    void setDashes(std::vector<double>&& dashes);

    /**
     * @brief The stroke width the dash pattern was computed for.
     *
     * The dash pattern is a fixed list of lengths, so a 6-3 pattern that reads well on a thin
     * stroke closes up into a solid-looking line on a thick one. The style is therefore computed
     * for the current pen thickness and the value it was computed for is stored here, so that
     * changing the thickness can recompute it. A value of 1.0 means "not tied to a thickness".
     */
    double getReferenceWidth() const;
    void setReferenceWidth(double width);

private:
    std::vector<double> dashes;

    /**
     * @brief Pen thickness the dashes were scaled for, see getReferenceWidth()
     */
    double referenceWidth = 1.0;
};
